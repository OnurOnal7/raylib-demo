#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "imgui.h"
#include "rlImGui.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define W 1000
#define H 650
#define NUM_ORBITS 8

typedef struct  {
    Vector3 color;
} Orbit;

Orbit orbits[NUM_ORBITS] = {
    { {1.0f, 0.0f, 0.0f} }, // red
    { {0.0f, 1.0f, 0.0f} }, // green
    { {0.0f, 0.0f, 1.0f} }, // blue
    { {0.9921f, 0.9843f, 0.8274f} }, // white
    { {1.0f, 1.0f, 0.0f} }, // yellow
    { {1.0f, 0.0f, 1.0f} }, // magenta
    { {0.0f, 1.0f, 1.0f} }, // cyan
    { {1.0f, 0.4f, 0.0f} }  // orange
};

Vector3 starts[NUM_ORBITS] = {
    {8.5f, 1.7f, 0.0f},
    {6.0f, 1.7f, 0.0f},
    {3.5f, 1.7f, 0.0f},
    {1.0f, 1.7f, 0.0f},
    {-1.0f, 1.7f, 0.0f}, 
    {-3.5f, 1.7f, 0.0f}, 
    {-6.0f, 1.7f, 0.0f}, 
    {-8.5f, 1.7f, 0.0f}, 
};

Vector3 axes[NUM_ORBITS] = {
    {0.7071, 0.7071, 0}, 
    {1, 0, 0}, 
    {-0.7071, 0.7071, 0}, 
    {0, 0, 1}, 
    {0, 0, 1},  
    {-0.7071, 0.7071, 0}, 
    {1, 0, 0},
    {0.7071, 0.7071, 0}, 
};

int main(void) {
    // Initialization
    InitWindow(W, H, "Raylib - Demo");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    rlImGuiSetup(true);

    // Function to create render textures
    auto createRenderTextures = [](int width, int height, RenderTexture2D& hdr, RenderTexture2D& bright, RenderTexture2D pingpong[2]) {
        // Clean up existing textures if they exist
        if (hdr.id != 0) {
            UnloadTexture(hdr.texture);
            UnloadRenderTexture(hdr);
        }
        if (bright.id != 0) {
            UnloadTexture(bright.texture);
            UnloadRenderTexture(bright);
        }
        for (int i = 0; i < 2; i++) {
            if (pingpong[i].id != 0) {
                UnloadTexture(pingpong[i].texture);
                UnloadRenderTexture(pingpong[i]);
            }
        }

        // Create HDR render texture
        hdr = LoadRenderTexture(width, height);
        UnloadTexture(hdr.texture);
        hdr.texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1);
        hdr.texture.width = width;
        hdr.texture.height = height;
        hdr.texture.mipmaps = 1;
        hdr.texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
        SetTextureWrap(hdr.texture, TEXTURE_WRAP_CLAMP);

        // Create bright render texture
        bright = LoadRenderTexture(width, height);
        UnloadTexture(bright.texture);
        bright.texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1);
        bright.texture.width = width;
        bright.texture.height = height;
        bright.texture.mipmaps = 1;
        bright.texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
        SetTextureWrap(bright.texture, TEXTURE_WRAP_CLAMP);

        // Bind the same FBO and attach both to it
        rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, hdr.id);
        rlBindFramebuffer(RL_READ_FRAMEBUFFER, hdr.id);
            rlFramebufferAttach(hdr.id, hdr.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
            rlFramebufferAttach(hdr.id, bright.texture.id, RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);
        
        // Activate two draw color buffers
        rlActiveDrawBuffers(2);

        // Unbind buffers
        rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, 0);
        rlBindFramebuffer(RL_READ_FRAMEBUFFER, 0);

        // Ping-pong FBOs for Gaussian blur
        for (int i = 0; i < 2; i++) {
            pingpong[i] = LoadRenderTexture(width, height);
            UnloadTexture(pingpong[i].texture);

            pingpong[i].texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1);
            pingpong[i].texture.width = width;
            pingpong[i].texture.height = height;
            pingpong[i].texture.mipmaps = 1;
            pingpong[i].texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
            SetTextureWrap(pingpong[i].texture, TEXTURE_WRAP_CLAMP);

            rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, pingpong[i].id);
            rlBindFramebuffer(RL_READ_FRAMEBUFFER, pingpong[i].id);
                rlFramebufferAttach(pingpong[i].id, pingpong[i].texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
                rlActiveDrawBuffers(1);

            rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, 0);
            rlBindFramebuffer(RL_READ_FRAMEBUFFER, 0);
        }
    };

    // Get current window dimensions
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();

    // Create render textures
    RenderTexture2D hdr, bright;
    RenderTexture2D pingpong[2];
    createRenderTextures(currentWidth, currentHeight, hdr, bright, pingpong);

    // Initialize camera
    Camera3D cam = { 0 };
    cam.position = Vector3{ -1.0f, 2.0f, -0.5f };
    cam.target   = Vector3{ 0.0f, 0.0f, 0.0f };
    cam.up       = Vector3{ 0.0f, 1.0f, 0.0f };
    cam.fovy     = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    
    // Load shaders
    Shader sh = LoadShader("resources/shaders/default.vs", "resources/shaders/phong.fs");
    int locLightPos  = GetShaderLocation(sh, "u_lightPos");
    int locLightCol  = GetShaderLocation(sh, "u_lightColor");
    int locLightInt  = GetShaderLocation(sh, "u_lightIntensity");
    int locEyePos    = GetShaderLocation(sh, "u_eyePos");
    int locAmb       = GetShaderLocation(sh, "u_ambientColor");
    int locSpec      = GetShaderLocation(sh, "u_specularColor");
    int locShine     = GetShaderLocation(sh, "u_shininess");
    sh.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(sh, "diffuseMap");
    sh.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(sh, "normalMap");

    Shader shEmis  = LoadShader("resources/shaders/default.vs", "resources/shaders/emissive.fs");
    int locEmis    = GetShaderLocation(shEmis, "u_emissiveColor");
    int locEmisInt = GetShaderLocation(shEmis, "u_emissiveIntensity");
    shEmis.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shEmis, "emissionMap");

    Shader shSky = LoadShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    int locRotView    = GetShaderLocation(shSky, "rotView");
    int locProjection = GetShaderLocation(shSky, "matProjection");
    shSky.locs[SHADER_LOC_MAP_CUBEMAP] = GetShaderLocation(shSky, "cubemap");

    Shader shHDR = LoadShader(NULL, "resources/shaders/hdr.fs");
    int locHdrGamma = GetShaderLocation(shHDR, "u_gamma");
    int locHdrExposure = GetShaderLocation(shHDR, "u_exposure");
    shHDR.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(shHDR, "hdrBuffer");
    shHDR.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shHDR, "bloomBlur");

    Shader shBlur = LoadShader(NULL, "resources/shaders/blur.fs");
    int locBlurHorizontal = GetShaderLocation(shBlur, "u_horizontal");
    shBlur.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(shBlur, "image");

    // Load cubemap imagesf
    Image px = LoadImage("resources/textures/right.jpg");
    Image nx = LoadImage("resources/textures/left.jpg"); 
    Image py = LoadImage("resources/textures/top.jpg"); 
    Image ny = LoadImage("resources/textures/bottom.jpg");
    Image pz = LoadImage("resources/textures/front.jpg");
    Image nz = LoadImage("resources/textures/back.jpg");

    int fw = px.width;
    int fh = px.height;

    float fwF = static_cast<float>(fw);
    float fhF = static_cast<float>(fh);
    
    // Create an image atlas for the cubemap
    Image atlas = GenImageColor(fw*4, fh*3, BLANK);

    // Common source‐rect
    Rectangle srcRec{ 0.0f, 0.0f, fwF, fhF };

    // Build all the destination rects with floats
    Rectangle dstRecs[6] = {
        { 0*fwF, 1*fhF, fwF, fhF },  
        { 2*fwF, 1*fhF, fwF, fhF },
        { 1*fwF, 0*fhF, fwF, fhF },
        { 1*fwF, 2*fhF, fwF, fhF },
        { 1*fwF, 1*fhF, fwF, fhF },
        { 3*fwF, 1*fhF, fwF, fhF }
    };

    Image faces[6] = { nx, px, py, ny, pz, nz };

    for (int i = 0; i < 6; i++) {
        ImageDraw(&atlas, faces[i], srcRec, dstRecs[i], WHITE);
    }

    Model sponzaModel = LoadModel("resources/objects/sponza.glb");
    for (int i = 0; i < sponzaModel.materialCount; i++) {
        sponzaModel.materials[i].shader = sh;        
    }

    Texture2D sunTex = LoadTexture("resources/textures/sun.jpg"); 

    // Load the sun image for CPU sampling
    Image sunImg = LoadImage("resources/textures/sun.jpg");
    Color c = GetImageColor(sunImg, sunImg.width / 2, sunImg.height / 2);
    float sunMask = (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b) / 255.0f;
    sunMask = pow(sunMask * 1.5f, 0.8f);
    UnloadImage(sunImg);

    TextureCubemap skyTex = LoadTextureCubemap(atlas, CUBEMAP_LAYOUT_CROSS_FOUR_BY_THREE);

    // Create skybox model
    Model skyModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    skyModel.materials[0].shader = shSky;
    skyModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = skyTex;
    
    Model orbitModels[NUM_ORBITS];
    Vector3 lightColor[NUM_ORBITS];
    float lightIntensity[NUM_ORBITS];
    Vector3 emissiveColor[NUM_ORBITS];
    float emissiveIntensity[NUM_ORBITS];
    
    // Create orbit models
    for (int i = 0; i < NUM_ORBITS; i++) {
        Mesh mesh = GenMeshSphere(0.2f, 64, 64);
        orbitModels[i] = LoadModelFromMesh(mesh);

        orbitModels[i].materials[0].shader = shEmis;
        orbitModels[i].materials[0].maps[MATERIAL_MAP_EMISSION].texture = sunTex;

        lightColor[i] = emissiveColor[i] = Vector3Scale(orbits[i].color, sunMask);
        lightIntensity[i] = emissiveIntensity[i] = 3.0f;
    }

    // Set colors
    Vector3 ambientColor = { 0.0001f, 0.0001f, 0.0001f };
    Vector3 specularColor = { 0.1f, 0.1f, 0.1f };
    float shininess = 32.0f;

    // Set HDR/bloom parameters
    float exposure = 1.0f;
    float gamma = 2.2f;

    float orbitSpinAngle = 0.0f;
    float orbitSpinSpeed = 0;
    Vector3 orbitScale = { 0.2f, 0.2f, 0.2f };
    
    while (!WindowShouldClose()) {
        // Check for window resize
        int newWidth = GetScreenWidth();
        int newHeight = GetScreenHeight();
        if (newWidth != currentWidth || newHeight != currentHeight) {
            currentWidth = newWidth;
            currentHeight = newHeight;
            createRenderTextures(currentWidth, currentHeight, hdr, bright, pingpong);
        }
 
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            UpdateCamera(&cam, CAMERA_FREE);

        // Only use rotation part of the view matrix
        Matrix view = GetCameraMatrix(cam);
        view.m12 = view.m13 = view.m14 = 0.0f;
        SetShaderValueMatrix(shSky, locRotView, view);

        // Build projection matrix
        float aspect = (float)currentWidth / (float)currentHeight;
        Matrix projection = MatrixPerspective(cam.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
        SetShaderValueMatrix(shSky, locProjection, projection);

        // Calculate rotation angles
        float dt = GetFrameTime();
        orbitSpinAngle += orbitSpinSpeed * dt;

        Vector3 positions[NUM_ORBITS];
        for (int i = 0; i < NUM_ORBITS; i++) {
            positions[i] = Vector3RotateByAxisAngle(starts[i], axes[i], orbitSpinAngle);
        }

        // Set shader values
        SetShaderValueV(sh, locLightPos, &positions[0], SHADER_UNIFORM_VEC3, NUM_ORBITS);
        SetShaderValueV(sh, locLightCol, &lightColor[0], SHADER_UNIFORM_VEC3, NUM_ORBITS);
        SetShaderValueV(sh, locLightInt, &lightIntensity[0], SHADER_UNIFORM_FLOAT, NUM_ORBITS);

        SetShaderValue(sh, locEyePos, &cam.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locAmb, &ambientColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locSpec, &specularColor, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locShine, &shininess, SHADER_UNIFORM_FLOAT);

        SetShaderValue(shHDR, locHdrGamma, &gamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shHDR, locHdrExposure, &exposure, SHADER_UNIFORM_FLOAT);

        SetShaderValue(shSky, locRotView, &view, SHADER_LOC_MATRIX_VIEW);

        for (int i = 0; i < NUM_ORBITS; i++) {
            SetShaderValue(shEmis, locEmis, &emissiveColor[i], SHADER_UNIFORM_VEC3);
            SetShaderValue(shEmis, locEmisInt, &emissiveIntensity[i], SHADER_UNIFORM_FLOAT);
        }

        //BeginDrawing();
        BeginTextureMode(hdr);
            rlActiveDrawBuffers(2);
            ClearBackground(BLACK);
            BeginMode3D(cam);

                BeginShaderMode(shSky);
                    rlDisableBackfaceCulling();
                    rlDisableDepthMask();

                    DrawModel(skyModel, cam.position, 1.0f, WHITE);

                    rlEnableBackfaceCulling();
                    rlEnableDepthMask();
                EndShaderMode();

                BeginShaderMode(sh);
                    DrawModel(sponzaModel, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
                EndShaderMode();

                BeginShaderMode(shEmis);
                    for (int i = 0; i < NUM_ORBITS; i++) {
                        SetShaderValue(shEmis, locEmis, &emissiveColor[i], SHADER_UNIFORM_VEC3);
                        SetShaderValue(shEmis, locEmisInt, &emissiveIntensity[i], SHADER_UNIFORM_FLOAT);
                        DrawModelEx(orbitModels[i], positions[i], axes[i], orbitSpinAngle, orbitScale, WHITE);
                    }
                EndShaderMode();

            EndMode3D();
                              
        EndTextureMode();
        //EndDrawing();

        bool firstIteration = true;
        int horizontal = 0;
        int amount = 10;
        
        for (int i = 0; i < amount; i++) {
            BeginTextureMode(pingpong[horizontal]);
                rlActiveDrawBuffers(1);

                BeginShaderMode(shBlur);
                    Texture2D src = firstIteration ? bright.texture : pingpong[(horizontal + 1) % 2].texture;

                    SetShaderValue(shBlur, locBlurHorizontal, &horizontal, SHADER_UNIFORM_INT);
                    SetShaderValueTexture(shBlur, shBlur.locs[SHADER_LOC_MAP_DIFFUSE], src);

                    Rectangle srcRect = { 0, 0, (float)currentWidth, -(float)currentHeight };
                    Vector2 position = { 0, 0 };
                    DrawTextureRec(src, srcRect, position, WHITE);  
                EndShaderMode();
                
            EndTextureMode();

            horizontal = (i + 1) % 2;
            firstIteration = false;
        }

        BeginDrawing();
        
            BeginShaderMode(shHDR);
                SetShaderValueTexture(shHDR, shHDR.locs[SHADER_LOC_MAP_DIFFUSE], hdr.texture);
                SetShaderValueTexture(shHDR, shHDR.locs[SHADER_LOC_MAP_EMISSION], pingpong[(horizontal + 1) % 2].texture);
                Rectangle hdrRect = { 0, 0, (float)currentWidth, -(float)currentHeight };
                Vector2 hdrPosition = { 0, 0 };
                DrawTextureRec(hdr.texture, hdrRect, hdrPosition, WHITE);
            EndShaderMode();

            rlImGuiBegin();
            
            if (ImGui::Begin("Controls")) {
                ImGui::DragFloat3("Camera Pos", (float*)&cam.position, 0.01f, -10.0f, 10.0f);
                ImGui::DragFloat3("Ambient Color", (float*)&ambientColor, 0.01f, 0.0f, 2.0f);
                ImGui::DragFloat3("Specular Color", (float*)&specularColor, 0.01f, 0.0f, 2.0f);
                ImGui::DragFloat("Gamma", &gamma, 0.01f, 1.0f, 3.0f);
            }

            ImGui::End();
            rlImGuiEnd();

            DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadShader(sh);
    UnloadShader(shEmis);
    UnloadShader(shSky);
    UnloadShader(shHDR);
    UnloadShader(shBlur);
    for (int i = 0; i < NUM_ORBITS; i++) UnloadModel(orbitModels[i]);
    UnloadTexture(sunTex);
    UnloadTexture(hdr.texture);
    UnloadTexture(bright.texture);
    UnloadTexture(pingpong[0].texture);
    UnloadTexture(pingpong[1].texture);
    UnloadImage(px);
    UnloadImage(nx);
    UnloadImage(py);
    UnloadImage(ny);
    UnloadImage(pz);
    UnloadImage(nz);
    UnloadImage(atlas);
    UnloadRenderTexture(hdr);

    CloseWindow();

    return 0;
}
