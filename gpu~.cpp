#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <cstring>

#include <m_pd.h>

static t_class *gpu_tilde_class;

// ─────────────────────────────────────
typedef struct _gpu_tilde {
    t_object obj;
    t_canvas *glist;
    t_sample sample;

    t_symbol *shader_name;
    t_symbol *shader_path;

    // parameters
    std::vector<float> parameters;

    GLuint program = 0;
    GLFWwindow *window = nullptr;
    int block_size = 64; // default
} t_gpu_tilde;

// ─────────────────────────────────────
std::string read_shader_file(t_gpu_tilde *x, const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        pd_error(x, "Failed to open shader file: %s", path.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ─────────────────────────────────────
GLuint create_compute_shader(t_gpu_tilde *x, const std::string &path) {
    std::string src = read_shader_file(x, path);
    if (src.empty()) {
        return 0;
    }

    const char *csrc = src.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        pd_error(x, "Compute shader compile error:\n%s", log);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        pd_error(x, "Shader program link error:\n%s", log);
        return 0;
    }

    glDeleteShader(shader);
    return program;
}

// ─────────────────────────────────────
void gpu_tilde_set_parameter(t_gpu_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    if (argc != 2) {
        pd_error(x, "Usage: set param_index value");
        return;
    }
    int index = atom_getint(argv);
    if (index - 1 < 0) {
        pd_error(x, "Parameter index must be >= 1");
        return;
    }

    float value = atom_getfloat(argv + 1);
    if (x->parameters.size() < index) {
        x->parameters.resize(index);
    }
    x->parameters[index - 1] = value;
    size_t paramCount = x->parameters.size();
}

// ─────────────────────────────────────
void gpu_tilde_reload(t_gpu_tilde *x) {
    if (!x->shader_path) {
        pd_error(x, "gpu~: No shader path set");
        return;
    }
    if (x->program) {
        glDeleteProgram(x->program);
        x->program = 0;
    }
    x->program = create_compute_shader(x, x->shader_path->s_name);
    if (!x->program) {
        pd_error(x, "gpu~: Failed to reload compute shader");
    } else {
        post("gpu~: Shader reloaded successfully");
    }
}

// ─────────────────────────────────────
t_int *gpu_tilde_perform(t_int *w) {
    t_gpu_tilde *x = (t_gpu_tilde *)w[1];
    int n = (int)w[2];         // samples por canal (block size)
    int chs = (int)w[3];       // número de canais
    float *in = (float *)w[4]; // buffer único contínuo: canal0, canal1, canal2, ...
    float *out = (float *)w[5];

    int totalSamples = n * chs;

    if (!x->program) {
        for (int i = 0; i < totalSamples; ++i) {
            out[i] = 0;
        }
        return (w + 6);
    }

    GLuint ssboIn, ssboOut, ssboParams;

    // Input buffer (binding = 0)
    glGenBuffers(1, &ssboIn);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIn);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSamples * sizeof(float), in, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboIn);

    // Output buffer (binding = 1)
    glGenBuffers(1, &ssboOut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboOut);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSamples * sizeof(float), nullptr, GL_STATIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboOut);

    // Parameters buffer (binding = 2)
    glGenBuffers(1, &ssboParams);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboParams);

    size_t paramCount = x->parameters.size();
    if (paramCount == 0) {
        float dummy = 0.0f; // valor default
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float), &dummy, GL_STATIC_DRAW);
    } else {
        glBufferData(GL_SHADER_STORAGE_BUFFER, paramCount * sizeof(float), x->parameters.data(),
                     GL_STATIC_DRAW);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParams);

    // Executa o shader com dispatch igual ao total de samples (todos canais contínuos)
    glUseProgram(x->program);
    glDispatchCompute(totalSamples, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Recupera resultado
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboOut);
    float *result = (float *)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (result) {
        memcpy(out, result, totalSamples * sizeof(float));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    } else {
        for (int i = 0; i < totalSamples; ++i) {
            out[i] = 0;
        }
    }

    // Cleanup
    glDeleteBuffers(1, &ssboIn);
    glDeleteBuffers(1, &ssboOut);
    glDeleteBuffers(1, &ssboParams);

    return (w + 6);
}

// ─────────────────────────────────────
void gpu_tilde_dsp(t_gpu_tilde *x, t_signal **sp) {
    int inCh = sp[0]->s_nchans;
    x->block_size = sp[0]->s_n;
    signal_setmultiout(&sp[1], inCh);
    dsp_add(gpu_tilde_perform, 5, x, sp[0]->s_n, sp[0]->s_nchans, sp[0]->s_vec, sp[1]->s_vec);
}

// ─────────────────────────────────────
void *gpu_tilde_new(t_symbol *s, int argc, t_atom *argv) {
    if (argc != 1 || argv[0].a_type != A_SYMBOL) {
        pd_error(nullptr, "Usage: gpu~ shader_path.glsl");
        return nullptr;
    }

    t_gpu_tilde *x = (t_gpu_tilde *)pd_new(gpu_tilde_class);
    x->shader_name = atom_getsymbol(argv);
    outlet_new(&x->obj, &s_signal);

    t_canvas *c = canvas_getcurrent();
    char dirresult[1024];
    char *realname = NULL;
    int fd = canvas_open(c, x->shader_name->s_name, ".glsl", dirresult, &realname, 1024, 0);
    if (fd < 0) {
        pd_error(x, "Shader file not found: %s", x->shader_path->s_name);
        return nullptr;
    }

    std::string path = std::string(dirresult) + "/" + x->shader_name->s_name + ".glsl";
    x->shader_path = gensym(path.c_str());

    if (!glfwInit()) {
        pd_error(x, "GLFW init failed");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    x->window = glfwCreateWindow(1, 1, "", nullptr, nullptr);
    if (!x->window) {
        pd_error(x, "Failed to create GLFW window");
        return nullptr;
    }

    glfwMakeContextCurrent(x->window);
    glewExperimental = GL_TRUE;
    glewInit();

    x->program = create_compute_shader(x, x->shader_path->s_name);
    if (!x->program) {
        pd_error(x, "Failed to compile compute shader");
    }

    return (void *)x;
}

// ─────────────────────────────────────
void gpu_tilde_free(t_gpu_tilde *x) {
    if (x->program) {
        glDeleteProgram(x->program);
    }
    if (x->window) {
        glfwDestroyWindow(x->window);
        glfwTerminate();
    }
}

// ─────────────────────────────────────
extern "C" void gpu_tilde_setup(void) {
    gpu_tilde_class =
        class_new(gensym("gpu~"), (t_newmethod)gpu_tilde_new, (t_method)gpu_tilde_free,
                  sizeof(t_gpu_tilde), CLASS_DEFAULT | CLASS_MULTICHANNEL, A_GIMME, 0);

    CLASS_MAINSIGNALIN(gpu_tilde_class, t_gpu_tilde, sample);
    class_addmethod(gpu_tilde_class, (t_method)gpu_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(gpu_tilde_class, (t_method)gpu_tilde_reload, gensym("reload"), A_NULL, 0);
    class_addmethod(gpu_tilde_class, (t_method)gpu_tilde_set_parameter, gensym("param"), A_GIMME,
                    0);
}
