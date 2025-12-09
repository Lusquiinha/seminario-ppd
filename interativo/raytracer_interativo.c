#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

// compilar: gcc -O3 -o rayview_omp raytracer_interativo.c -lm -fopenmp `sdl2-config --cflags --libs`

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_RAY_DEPTH 5
#define WIDTH 1280
#define HEIGHT 720

/* ======== Estruturas ======== */

typedef struct {
    float x, y, z;
} Vec3f;

typedef struct {
    Vec3f center;
    float radius;
    float radius2;
    Vec3f surfaceColor;
    Vec3f emissionColor;
    float reflection;
    float transparency;
} Sphere;

typedef struct {
    Vec3f pos;       // Posição da câmera
    Vec3f forward;   // Direção para frente
    Vec3f right;     // Direção para direita
    Vec3f up;        // Direção para cima
    float pitch;     // Rotação vertical (radianos)
    float yaw;       // Rotação horizontal (radianos)
} Camera;

/* ======== Funções de vetor ======== */

static inline Vec3f vec_add(Vec3f a, Vec3f b) {
    Vec3f r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
}

static inline Vec3f vec_sub(Vec3f a, Vec3f b) {
    Vec3f r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
}

static inline Vec3f vec_mul_scalar(Vec3f a, float s) {
    Vec3f r = { a.x * s, a.y * s, a.z * s };
    return r;
}

static inline Vec3f vec_mul(Vec3f a, Vec3f b) {
    Vec3f r = { a.x * b.x, a.y * b.y, a.z * b.z };
    return r;
}

static inline float vec_dot(Vec3f a, Vec3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float vec_length(Vec3f a) {
    return sqrtf(vec_dot(a, a));
}

static inline Vec3f vec_normalize(Vec3f a) {
    float len = vec_length(a);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        Vec3f r = { a.x * inv, a.y * inv, a.z * inv };
        return r;
    } else {
        return a;
    }
}

static inline Vec3f vec_neg(Vec3f a) {
    Vec3f r = { -a.x, -a.y, -a.z };
    return r;
}

static inline Vec3f vec_cross(Vec3f a, Vec3f b) {
    Vec3f r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return r;
}

/* ======== Utilitários ======== */

static inline float mixf(float a, float b, float mix) {
    return b * mix + a * (1.0f - mix);
}

static inline float clampf(float x, float a, float b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

/* ======== Câmera ======== */

// Inicializa a câmera
void camera_init(Camera *cam, Vec3f pos) {
    cam->pos = pos;
    cam->pitch = 0.0f;
    cam->yaw = 0.0f;
    
    // Atualiza vetores direcionais
    cam->forward.x = 0.0f;
    cam->forward.y = 0.0f;
    cam->forward.z = -1.0f;
    
    cam->right.x = 1.0f;
    cam->right.y = 0.0f;
    cam->right.z = 0.0f;
    
    cam->up.x = 0.0f;
    cam->up.y = 1.0f;
    cam->up.z = 0.0f;
}

// Atualiza os vetores da câmera com base em pitch e yaw
void camera_update_vectors(Camera *cam) {
    // Calcula forward baseado em yaw e pitch
    cam->forward.x = cosf(cam->pitch) * sinf(cam->yaw);
    cam->forward.y = sinf(cam->pitch);
    cam->forward.z = -cosf(cam->pitch) * cosf(cam->yaw);
    cam->forward = vec_normalize(cam->forward);
    
    // Calcula right (produto vetorial de forward com world up)
    Vec3f worldUp = { 0.0f, 1.0f, 0.0f };
    cam->right = vec_cross(cam->forward, worldUp);
    cam->right = vec_normalize(cam->right);
    
    // Calcula up (produto vetorial de right com forward)
    cam->up = vec_cross(cam->right, cam->forward);
    cam->up = vec_normalize(cam->up);
}

// Move a câmera
void camera_move(Camera *cam, Vec3f direction, float speed) {
    cam->pos = vec_add(cam->pos, vec_mul_scalar(direction, speed));
}

// Rotaciona a câmera
void camera_rotate(Camera *cam, float dpitch, float dyaw) {
    cam->pitch += dpitch;
    cam->yaw += dyaw;
    
    // Limita pitch para evitar flip
    const float limit = M_PI / 2.0f - 0.01f;
    if (cam->pitch > limit) cam->pitch = limit;
    if (cam->pitch < -limit) cam->pitch = -limit;
    
    camera_update_vectors(cam);
}

/* ======== Interseção raio-esfera ======== */

bool sphere_intersect(const Sphere *s, Vec3f rayorig, Vec3f raydir, float *t0, float *t1) {
    Vec3f L = vec_sub(s->center, rayorig);
    float tca = vec_dot(L, raydir);
    if (tca < 0) return false;
    float d2 = vec_dot(L, L) - tca * tca;
    if (d2 > s->radius2) return false;
    float thc = sqrtf(s->radius2 - d2);
    *t0 = tca - thc;
    *t1 = tca + thc;
    return true;
}

/* ======== Ray tracing recursivo ======== */

Vec3f trace(Vec3f rayorig,
            Vec3f raydir,
            const Sphere *spheres,
            int numSpheres,
            int depth)
{
    float tnear = INFINITY;
    const Sphere *sphere = NULL;

    // Encontra a esfera mais próxima intersectada
    for (int i = 0; i < numSpheres; ++i) {
        float t0 = INFINITY, t1 = INFINITY;
        if (sphere_intersect(&spheres[i], rayorig, raydir, &t0, &t1)) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }

    // Nenhuma interseção: cor de fundo (céu azul)
    if (!sphere) {
        Vec3f bg = { 0.5f, 0.7f, 0.9f };
        return bg;
    }

    Vec3f surfaceColor = { 0.0f, 0.0f, 0.0f };

    // Ponto e normal na interseção
    Vec3f phit = vec_add(rayorig, vec_mul_scalar(raydir, tnear));
    Vec3f nhit = vec_sub(phit, sphere->center);
    nhit = vec_normalize(nhit);

    float bias = 1e-4f;
    bool inside = false;

    if (vec_dot(raydir, nhit) > 0.0f) {
        nhit = vec_neg(nhit);
        inside = true;
    }

    // Reflexão / Refração
    if ((sphere->transparency > 0.0f || sphere->reflection > 0.0f) &&
        depth < MAX_RAY_DEPTH)
    {
        float facingratio = -vec_dot(raydir, nhit);
        float fresneleffect = mixf(powf(1.0f - facingratio, 3.0f), 1.0f, 0.1f);

        // Raio refletido
        Vec3f refldir = vec_sub(raydir, vec_mul_scalar(nhit, 2.0f * vec_dot(raydir, nhit)));
        refldir = vec_normalize(refldir);
        Vec3f reflection = trace(vec_add(phit, vec_mul_scalar(nhit, bias)),
                                 refldir, spheres, numSpheres, depth + 1);

        Vec3f refraction = { 0.0f, 0.0f, 0.0f };

        if (sphere->transparency > 0.0f) {
            float ior = 1.1f;
            float eta = inside ? ior : (1.0f / ior);
            float cosi = -vec_dot(nhit, raydir);
            float k = 1.0f - eta * eta * (1.0f - cosi * cosi);

            if (k >= 0.0f) {
                Vec3f refrdir = vec_add(vec_mul_scalar(raydir, eta),
                                        vec_mul_scalar(nhit, eta * cosi - sqrtf(k)));
                refrdir = vec_normalize(refrdir);
                refraction = trace(vec_sub(phit, vec_mul_scalar(nhit, bias)),
                                   refrdir, spheres, numSpheres, depth + 1);
            }
        }

        // Combinação
        Vec3f term1 = vec_mul_scalar(reflection, fresneleffect);
        Vec3f term2 = vec_mul_scalar(refraction,
                                     (1.0f - fresneleffect) * sphere->transparency);
        surfaceColor = vec_mul(vec_add(term1, term2), sphere->surfaceColor);
    } else {
        // Iluminação direta (difusa) usando esferas emissoras como luz
        for (int i = 0; i < numSpheres; ++i) {
            if (spheres[i].emissionColor.x > 0.0f ||
                spheres[i].emissionColor.y > 0.0f ||
                spheres[i].emissionColor.z > 0.0f)
            {
                Vec3f transmission = { 1.0f, 1.0f, 1.0f };
                Vec3f lightDirection = vec_sub(spheres[i].center, phit);
                lightDirection = vec_normalize(lightDirection);

                // Ray casting para sombra
                for (int j = 0; j < numSpheres; ++j) {
                    if (i != j) {
                        float t0, t1;
                        if (sphere_intersect(&spheres[j],
                                             vec_add(phit, vec_mul_scalar(nhit, bias)),
                                             lightDirection,
                                             &t0, &t1))
                        {
                            Vec3f zero = { 0.0f, 0.0f, 0.0f };
                            transmission = zero;
                            break;
                        }
                    }
                }

                float dotLN = vec_dot(nhit, lightDirection);
                if (dotLN > 0.0f) {
                    Vec3f contrib = vec_mul(vec_mul(sphere->surfaceColor, transmission),
                                            spheres[i].emissionColor);
                    contrib = vec_mul_scalar(contrib, dotLN);
                    surfaceColor = vec_add(surfaceColor, contrib);
                }
            }
        }
    }

    // Soma emissão própria do objeto
    surfaceColor = vec_add(surfaceColor, sphere->emissionColor);
    return surfaceColor;
}

/* ======== Renderização ======== */

void render(Vec3f *image,
            unsigned width,
            unsigned height,
            const Sphere *spheres,
            int numSpheres,
            const Camera *cam)
{
    float invWidth  = 1.0f / (float)width;
    float invHeight = 1.0f / (float)height;
    float fov = 30.0f;
    float aspectratio = (float)width / (float)height;
    float angle = tanf(M_PI * 0.5f * fov / 180.0f);

    #pragma omp parallel for schedule(dynamic)
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            float xx = (2.0f * ((x + 0.5f) * invWidth)  - 1.0f) * angle * aspectratio;
            float yy = (1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

            // Calcula direção do raio baseada na câmera
            Vec3f raydir = vec_add(cam->forward,
                                   vec_add(vec_mul_scalar(cam->right, xx),
                                          vec_mul_scalar(cam->up, yy)));
            raydir = vec_normalize(raydir);

            // Origem do raio é a posição da câmera
            image[y * width + x] = trace(cam->pos, raydir, spheres, numSpheres, 0);
        }
    }
}

/* ======== Cena ======== */

// Cria a cena com esferas
int setup_scene(Sphere **spheres_out) {
    int numSpheres = 6;
    Sphere *spheres = (Sphere *)malloc(numSpheres * sizeof(Sphere));
    
    // Esfera 1: Vermelha opaca
    spheres[0].center = (Vec3f){ 0.0f, 0.0f, -10.0f };
    spheres[0].radius = 1.5f;
    spheres[0].radius2 = 1.5f * 1.5f;
    spheres[0].surfaceColor = (Vec3f){ 1.0f, 0.2f, 0.2f };
    spheres[0].emissionColor = (Vec3f){ 0.0f, 0.0f, 0.0f };
    spheres[0].reflection = 0.5f;
    spheres[0].transparency = 0.0f;
    
    // Esfera 2: Vidro transparente
    spheres[1].center = (Vec3f){ 3.0f, 0.0f, -8.0f };
    spheres[1].radius = 1.2f;
    spheres[1].radius2 = 1.2f * 1.2f;
    spheres[1].surfaceColor = (Vec3f){ 0.9f, 0.9f, 0.9f };
    spheres[1].emissionColor = (Vec3f){ 0.0f, 0.0f, 0.0f };
    spheres[1].reflection = 0.9f;
    spheres[1].transparency = 0.9f;
    
    // Esfera 3: Azul metálica
    spheres[2].center = (Vec3f){ -3.0f, 0.5f, -7.0f };
    spheres[2].radius = 1.0f;
    spheres[2].radius2 = 1.0f * 1.0f;
    spheres[2].surfaceColor = (Vec3f){ 0.2f, 0.3f, 0.8f };
    spheres[2].emissionColor = (Vec3f){ 0.0f, 0.0f, 0.0f };
    spheres[2].reflection = 0.7f;
    spheres[2].transparency = 0.0f;
    
    // Esfera 4: Chão (esfera gigante)
    spheres[3].center = (Vec3f){ 0.0f, -1004.0f, -10.0f };
    spheres[3].radius = 1000.0f;
    spheres[3].radius2 = 1000.0f * 1000.0f;
    spheres[3].surfaceColor = (Vec3f){ 0.4f, 0.6f, 0.4f };
    spheres[3].emissionColor = (Vec3f){ 0.0f, 0.0f, 0.0f };
    spheres[3].reflection = 0.1f;
    spheres[3].transparency = 0.0f;
    
    // Esfera 5: Luz amarela
    spheres[4].center = (Vec3f){ -5.0f, 10.0f, -5.0f };
    spheres[4].radius = 1.0f;
    spheres[4].radius2 = 1.0f * 1.0f;
    spheres[4].surfaceColor = (Vec3f){ 1.0f, 1.0f, 1.0f };
    spheres[4].emissionColor = (Vec3f){ 2.0f, 2.0f, 1.5f };
    spheres[4].reflection = 0.0f;
    spheres[4].transparency = 0.0f;
    
    // Esfera 6: Luz azul
    spheres[5].center = (Vec3f){ 5.0f, 8.0f, -8.0f };
    spheres[5].radius = 0.8f;
    spheres[5].radius2 = 0.8f * 0.8f;
    spheres[5].surfaceColor = (Vec3f){ 1.0f, 1.0f, 1.0f };
    spheres[5].emissionColor = (Vec3f){ 1.0f, 1.5f, 2.5f };
    spheres[5].reflection = 0.0f;
    spheres[5].transparency = 0.0f;
    
    *spheres_out = spheres;
    return numSpheres;
}

/* ======== Main ======== */

int main(int argc, char *argv[]) {
    // Inicializa SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Erro ao inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    // Cria janela
    SDL_Window *window = SDL_CreateWindow(
        "Ray Tracer Interativo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        fprintf(stderr, "Erro ao criar janela: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Cria renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Erro ao criar renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Cria textura para o buffer de imagem
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT
    );
    
    if (!texture) {
        fprintf(stderr, "Erro ao criar textura: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Aloca buffer de imagem
    Vec3f *image = (Vec3f *)malloc(WIDTH * HEIGHT * sizeof(Vec3f));
    Uint32 *pixels = (Uint32 *)malloc(WIDTH * HEIGHT * sizeof(Uint32));
    
    // Configura cena
    Sphere *spheres;
    int numSpheres = setup_scene(&spheres);
    
    // Inicializa câmera
    Camera cam;
    Vec3f startPos = { 0.0f, 2.0f, 5.0f };
    camera_init(&cam, startPos);
    
    // Variáveis de controle
    bool running = true;
    bool keys[SDL_NUM_SCANCODES] = { false };
    float moveSpeed = 0.1f;
    float rotSpeed = 0.05f;
    bool mouseCaptured = false;
    
    printf("Controles:\n");
    printf("  W/S: Mover para frente/trás\n");
    printf("  A/D: Mover para esquerda/direita\n");
    printf("  Q/E: Mover para cima/baixo\n");
    printf("  Setas: Rotacionar câmera\n");
    printf("  ESC: Sair\n");
    printf("  Clique: Capturar/Liberar mouse\n\n");
    
    Uint32 frameStart, frameTime;
    int frameCount = 0;
    Uint32 fpsTimer = SDL_GetTicks();
    float avgFPS = 0.0f;
    
    // Loop principal
    while (running) {
        frameStart = SDL_GetTicks();
        
        // ======== Processa eventos ========
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.scancode < SDL_NUM_SCANCODES) {
                        keys[event.key.keysym.scancode] = true;
                    }
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                    
                case SDL_KEYUP:
                    if (event.key.keysym.scancode < SDL_NUM_SCANCODES) {
                        keys[event.key.keysym.scancode] = false;
                    }
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    mouseCaptured = !mouseCaptured;
                    SDL_SetRelativeMouseMode(mouseCaptured ? SDL_TRUE : SDL_FALSE);
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (mouseCaptured) {
                        float sensitivity = 0.002f;
                        camera_rotate(&cam, -event.motion.yrel * sensitivity,
                                          event.motion.xrel * sensitivity);
                    }
                    break;
            }
        }
        
        // ======== Processa input contínuo ========
        
        // Movimento WASD
        if (keys[SDL_SCANCODE_W]) {
            camera_move(&cam, cam.forward, moveSpeed);
        }
        if (keys[SDL_SCANCODE_S]) {
            camera_move(&cam, cam.forward, -moveSpeed);
        }
        if (keys[SDL_SCANCODE_A]) {
            camera_move(&cam, cam.right, -moveSpeed);
        }
        if (keys[SDL_SCANCODE_D]) {
            camera_move(&cam, cam.right, moveSpeed);
        }
        
        // Movimento vertical QE
        if (keys[SDL_SCANCODE_Q]) {
            camera_move(&cam, cam.up, -moveSpeed);
        }
        if (keys[SDL_SCANCODE_E]) {
            camera_move(&cam, cam.up, moveSpeed);
        }
        
        // Rotação com setas
        if (keys[SDL_SCANCODE_LEFT]) {
            camera_rotate(&cam, 0.0f, -rotSpeed);
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            camera_rotate(&cam, 0.0f, rotSpeed);
        }
        if (keys[SDL_SCANCODE_UP]) {
            camera_rotate(&cam, rotSpeed, 0.0f);
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            camera_rotate(&cam, -rotSpeed, 0.0f);
        }
        
        // ======== Renderiza frame ========
        render(image, WIDTH, HEIGHT, spheres, numSpheres, &cam);
        
        // ======== Converte Vec3f para RGB8 ========
        #pragma omp parallel for
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            Uint8 r = (Uint8)(clampf(image[i].x, 0.0f, 1.0f) * 255.0f);
            Uint8 g = (Uint8)(clampf(image[i].y, 0.0f, 1.0f) * 255.0f);
            Uint8 b = (Uint8)(clampf(image[i].z, 0.0f, 1.0f) * 255.0f);
            pixels[i] = (r << 16) | (g << 8) | b;
        }
        
        // ======== Atualiza textura e desenha ========
        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // ======== Controle de frame rate ========
        frameTime = SDL_GetTicks() - frameStart;
        // if (frameTime < 16) { // ~60 FPS
        //     SDL_Delay(16 - frameTime);
        // }
        
        // Mostra FPS no terminal
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - fpsTimer >= 1000) {
            avgFPS = frameCount * 1000.0f / (currentTime - fpsTimer);
            printf("\rFPS: %.2f | Frame Time: %.2f ms   ", avgFPS, 1000.0f / avgFPS);
            fflush(stdout);
            frameCount = 0;
            fpsTimer = currentTime;
        }
    }
    
    // ======== Limpeza ========
    printf("\nEncerrando...\n");
    
    free(pixels);
    free(image);
    free(spheres);
    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

