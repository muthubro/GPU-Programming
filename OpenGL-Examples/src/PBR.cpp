#include "PBR.h"
#include "Lighting.h"

#include <stb_image/stb_image.h>

static const double PI = 3.14159265359;

static const uint32_t SCR_WIDTH = 1280, SCR_HEIGHT = 720;
static const uint32_t SHADOW_WIDTH = 720, SHADOW_HEIGHT = 720;

PBR::PBR()
    : m_Camera(glm::perspectiveFov(glm::radians(45.0f), float(SCR_WIDTH), float(SCR_HEIGHT), 0.1f, 50000.0f))
{
    m_Camera.SetPitch(0.0f);
    m_Camera.SetYaw(0.0f);
    m_Camera.UpdateCamera();
}

PBR::~PBR()
{
}

static uint32_t LoadTexture(char const* path, bool hdr = false, bool gammaCorrection = false)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    if (hdr)
        stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    void* data;
    if (hdr)
        data = stbi_loadf(path, &width, &height, &channels, 0);
    else
        data = stbi_load(path, &width, &height, &channels, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (channels == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (channels == 3)
        {
            if (hdr)
                internalFormat = GL_RGB16F;
            else
                internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (channels == 4)
        {
            if (hdr)
                internalFormat = GL_RGBA16F;
            else
                internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, data);
        if (!hdr)
            glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hdr ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void PBR::OnAttach()
{
    static float cubeVertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
        // bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
         1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
    };
    static float quadVertices[] =
    {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f
    };

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Generate sphere
    glCreateVertexArrays(1, &m_SphereVAO);
    glBindVertexArray(m_SphereVAO);

    static const uint32_t X_SEGMENTS = 64, Y_SEGMENTS = 64;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec2> uv;
    for (uint32_t y = 0; y <= Y_SEGMENTS; y++)
    {
        for (uint32_t x = 0; x <= X_SEGMENTS; x++)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            float xpos = (float)(cos(2.0 * PI * xSegment) * sin(PI * ySegment));
            float ypos = (float)(cos(PI * ySegment));
            float zpos = (float)(sin(2.0 * PI * xSegment) * sin(PI * ySegment));

            positions.emplace_back(xpos, ypos, zpos);
            normals.emplace_back(xpos, ypos, zpos);
            tangents.emplace_back(-(float)sin(2.0 * PI * xSegment), 0, (float)(cos(2.0 * PI * xSegment)));
            uv.emplace_back(xSegment, ySegment);
        }
    }

    std::vector<uint32_t> indices;
    bool oddRow = false;
    for (uint32_t y = 0; y < Y_SEGMENTS; y++)
    {
        if (!oddRow)
        {
            for (uint32_t x = 0; x <= X_SEGMENTS; x++)
            {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; x--)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    m_SphereIndexCount = indices.size();

    std::vector<float> data;
    for (uint32_t i = 0; i < positions.size(); i++)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);

        if (!normals.empty())
        {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }

        if (!tangents.empty())
        {
            data.push_back(tangents[i].x);
            data.push_back(tangents[i].y);
            data.push_back(tangents[i].z);
        }

        if (!uv.empty())
        {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }
    }

    uint32_t sphereVBO;
    glCreateBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glNamedBufferData(sphereVBO, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    float stride = (3 + 3 + 3 + 2) * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glCreateBuffers(1, &m_SphereIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereIBO);
    glNamedBufferData(m_SphereIBO, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    m_SphereAlbedoMap = LoadTexture("assets/textures/pirate-gold-bl/pirate-gold_albedo.png");
    m_SphereAOMap = LoadTexture("assets/textures/pirate-gold-bl/pirate-gold_ao.png");
    m_SphereMetallicMap = LoadTexture("assets/textures/pirate-gold-bl/pirate-gold_metallic.png");
    m_SphereNormalMap = LoadTexture("assets/textures/pirate-gold-bl/pirate-gold_normal-ogl.png");
    m_SphereRoughnessMap = LoadTexture("assets/textures/pirate-gold-bl/pirate-gold_roughness.png");
    m_SphereHeightMap = LoadTexture("assets/textures/pirate-gold-bl/pirate-gold_height.png");

    m_PBRShader = Shader::FromGLSLTextFiles("assets/shaders/pbr.vert.glsl", "assets/shaders/pbr.frag.glsl");

    // Cube
    glCreateVertexArrays(1, &m_CubeVAO);
    glBindVertexArray(m_CubeVAO);

    uint32_t cubeVBO;
    glCreateBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glNamedBufferData(cubeVBO, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    m_SkyboxShader = Shader::FromGLSLTextFiles("assets/shaders/skybox.vert.glsl", "assets/shaders/skybox.frag.glsl");

    // Equirectangular to Cubemap
    glCreateFramebuffers(1, &m_EnvironmentFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentFBO);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_CubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapTexture);
    for (uint32_t i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_CubemapTexture, 0);
    }
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_CubemapTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glCreateRenderbuffers(1, &m_CubemapDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_CubemapDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CubemapDepthRBO);

    GLCORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer incomplete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_EquirectangularToCubemapShader = Shader::FromGLSLTextFiles("assets/shaders/cubemap.vert.glsl", "assets/shaders/equirectangularToCubemap.frag.glsl");

    uint32_t hdrTexture = LoadTexture("assets/textures/Newport_Loft/Newport_Loft_Ref.hdr", true);
    EquirectangularToCubemap(hdrTexture);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapTexture);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Irradiance
    glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentFBO);
    
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_IrradianceTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceTexture);
    for (uint32_t i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceTexture, 0);
    }
    glTextureParameteri(m_IrradianceTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_IrradianceTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_IrradianceTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_IrradianceTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_IrradianceTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, m_CubemapDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    m_IrradianceShader = Shader::FromGLSLTextFiles("assets/shaders/cubemap.vert.glsl", "assets/shaders/irradiance.frag.glsl");

    GenerateIrradiance(m_CubemapTexture);

    // Prefiltered Environment Map
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_PrefilteredEnvMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilteredEnvMap);
    for (uint32_t i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 256, 256, 0, GL_RGB, GL_FLOAT, nullptr);

    glTextureParameteri(m_PrefilteredEnvMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_PrefilteredEnvMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_PrefilteredEnvMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_PrefilteredEnvMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_PrefilteredEnvMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    m_PrefilterShader = Shader::FromGLSLTextFiles("assets/shaders/cubemap.vert.glsl", "assets/shaders/prefilter.frag.glsl");

    GeneratePrefilteredEnvMap(m_CubemapTexture);

    // BRDF LUT
    glCreateVertexArrays(1, &m_QuadVAO);
    glBindVertexArray(m_QuadVAO);
    
    uint32_t quadVBO;
    glCreateBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glNamedBufferData(quadVBO, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glCreateTextures(GL_TEXTURE_2D, 1, &m_BRDFLUT);
    glBindTexture(GL_TEXTURE_2D, m_BRDFLUT);
    glTextureStorage2D(m_BRDFLUT, 1, GL_RG16F, 512, 512);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_BRDFIntegrationShader = Shader::FromGLSLTextFiles("assets/shaders/brdf.vert.glsl", "assets/shaders/brdf.frag.glsl");

    GenerateBRDFIntegration(m_CubemapTexture);

    m_QuadShader = Shader::FromGLSLTextFiles("assets/shaders/quad.vert.glsl", "assets/shaders/quad.frag.glsl");
}

void PBR::OnDetach()
{
    glDeleteBuffers(1, &m_SphereVAO);
    glDeleteBuffers(1, &m_SphereIBO);
}

void PBR::OnEvent(GLCore::Event& e)
{
    m_Camera.OnEvent(e);
}

void PBR::EquirectangularToCubemap(uint32_t equirectangularMap)
{
    static glm::mat4 viewMatrices[] =
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f, 0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentFBO);
    glViewport(0, 0, 512, 512);

    uint32_t shader = m_EquirectangularToCubemapShader->GetRendererID();
    glUseProgram(shader);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindTextureUnit(0, equirectangularMap);
    glUniform1i(glGetUniformLocation(shader, "u_EquirectangularMap"), 0);

    glBindVertexArray(m_CubeVAO);

    for (uint32_t i = 0; i < 6; i++)
    {
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_View"), 1, GL_FALSE, glm::value_ptr(viewMatrices[i]));

        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void PBR::GenerateBRDFIntegration(uint32_t environment)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BRDFLUT, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindRenderbuffer(GL_RENDERBUFFER, m_CubemapDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);

    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uint32_t shader = m_BRDFIntegrationShader->GetRendererID();
    glUseProgram(shader);

    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void PBR::GenerateIrradiance(uint32_t environment)
{
    static glm::mat4 viewMatrices[] =
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f, 0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentFBO);
    glViewport(0, 0, 32, 32);

    uint32_t shader = m_IrradianceShader->GetRendererID();
    glUseProgram(shader);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindTextureUnit(0, environment);
    glUniform1i(glGetUniformLocation(shader, "u_Environment"), 0);

    glBindVertexArray(m_CubeVAO);

    for (uint32_t i = 0; i < 6; i++)
    {
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_View"), 1, GL_FALSE, glm::value_ptr(viewMatrices[i]));

        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void PBR::GeneratePrefilteredEnvMap(uint32_t environment)
{
    static glm::mat4 viewMatrices[] =
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f, 0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    static const uint32_t MAX_MIP_COUNT = 5;

    glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentFBO);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0, 0, 256, 256);

    uint32_t shader = m_PrefilterShader->GetRendererID();
    glUseProgram(shader);

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindTextureUnit(0, environment);
    glUniform1i(glGetUniformLocation(shader, "u_EnvironmentMap"), 0);

    glBindVertexArray(m_CubeVAO);

    for (uint32_t mip = 0; mip < MAX_MIP_COUNT; mip++)
    {
        uint32_t mipWidth  = 256 * pow(0.5, mip);
        uint32_t mipHeight = 256 * pow(0.5, mip);
    
        glBindRenderbuffer(GL_RENDERBUFFER, m_CubemapDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
    
        float roughness = (float)mip / (float)(MAX_MIP_COUNT - 1);
        glUniform1f(glGetUniformLocation(shader, "u_Roughness"), roughness);
        for (uint32_t i = 0; i < 6; i++)
        {
            glUniformMatrix4fv(glGetUniformLocation(shader, "u_View"), 1, GL_FALSE, glm::value_ptr(viewMatrices[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilteredEnvMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void PBR::OnUpdate(GLCore::Timestep ts)
{
    static glm::vec3 lightPositions[] = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3(10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3(10.0f, -10.0f, 10.0f),
    };
    static glm::vec3 lightColors[] = {
        glm::vec3(1000.0f, 1000.0f, 1000.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewProj = m_Camera.GetViewProjection();
    glm::vec3 viewPos = m_Camera.GetPosition();

    glm::mat4 model(1.0f);
    glm::mat3 normalModel(1.0f);
    uint32_t shader = 0;

    static int rows = 7;
    static int columns = 7;
    static float spacing = 2.5f;

    shader = m_PBRShader->GetRendererID();
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform3f(glGetUniformLocation(shader, "u_ViewPos"), viewPos.x, viewPos.y, viewPos.z);
    
    glUniform1i(glGetUniformLocation(shader, "u_TextureToggle"), m_Textured);
    if (m_Textured)
    {
        glBindTextureUnit(0, m_SphereAlbedoMap);
        glUniform1i(glGetUniformLocation(shader, "u_AlbedoMap"), 0);
        glBindTextureUnit(1, m_SphereMetallicMap);
        glUniform1i(glGetUniformLocation(shader, "u_MetallicMap"), 1);
        glBindTextureUnit(2, m_SphereNormalMap);
        glUniform1i(glGetUniformLocation(shader, "u_NormalMap"), 2);
        glBindTextureUnit(3, m_SphereRoughnessMap);
        glUniform1i(glGetUniformLocation(shader, "u_RoughnessMap"), 3);
        glBindTextureUnit(4, m_SphereMetallicMap);
        glUniform1i(glGetUniformLocation(shader, "u_AOMap"), 4);
        glBindTextureUnit(5, m_SphereHeightMap);
        glUniform1i(glGetUniformLocation(shader, "u_HeightMap"), 5);

        glUniform2f(glGetUniformLocation(shader, "u_TilingFactor"), 9.0f, 5.0f);
    }
    else
    {
        glUniform3f(glGetUniformLocation(shader, "u_Albedo"), 0.5f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(shader, "u_AO"), 1.0f);
    }

    glBindVertexArray(m_SphereVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereIBO);

    // Spheres
    for (int row = 0; row < rows; row++)
    {
        if (!m_Textured)
            glUniform1f(glGetUniformLocation(shader, "u_Metallic"), (float)row / (float)rows);

        for (int col = 0; col < columns; col++)
        {
            if (!m_Textured)
                glUniform1f(glGetUniformLocation(shader, "u_Roughness"), glm::clamp((float)col / (float)columns, 0.05f, 1.0f));

            model = glm::translate(glm::mat4(1.0f), glm::vec3(
                (col - columns / 2) * spacing,
                (row - rows / 2) * spacing,
                0.0f
            ));
            normalModel = glm::transpose(glm::inverse(glm::mat3(model)));
            glUniformMatrix4fv(glGetUniformLocation(shader, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix3fv(glGetUniformLocation(shader, "u_NormalModel"), 1, GL_FALSE, glm::value_ptr(normalModel));

            glDrawElements(GL_TRIANGLE_STRIP, m_SphereIndexCount, GL_UNSIGNED_INT, nullptr);
        }
    }

    glUniform1f(glGetUniformLocation(shader, "u_IBL"), m_IBL);

    if (m_IBL)
    {
        glBindTextureUnit(5, m_BRDFLUT);
        glUniform1i(glGetUniformLocation(shader, "u_BRDFLUT"), 5);

        glBindTextureUnit(6, m_IrradianceTexture);
        glUniform1i(glGetUniformLocation(shader, "u_IrradianceMap"), 6);

        glBindTextureUnit(7, m_PrefilteredEnvMap);
        glUniform1i(glGetUniformLocation(shader, "u_PrefilterMap"), 7);
    }

    glUniform1f(glGetUniformLocation(shader, "u_Exposure"), m_Exposure);

    // Light sources
    for (uint32_t i = 0; i < 4; i++)
    {
        glUniform3f(glGetUniformLocation(shader, ("u_LightPositions[" + std::to_string(i) + "]").c_str()), lightPositions[i].x, lightPositions[i].y, lightPositions[i].z);
        glUniform3f(glGetUniformLocation(shader, ("u_LightColors[" + std::to_string(i) + "]").c_str()), lightColors[i].r, lightColors[i].g, lightColors[i].b);

        model = glm::translate(glm::mat4(1.0f), lightPositions[i]);
        model = glm::scale(model, glm::vec3(0.5f));
        normalModel = glm::transpose(glm::inverse(glm::mat3(model)));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(glGetUniformLocation(shader, "u_NormalModel"), 1, GL_FALSE, glm::value_ptr(normalModel));

        glDrawElements(GL_TRIANGLE_STRIP, m_SphereIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    char* argv[] = { m_IBL ? "1" : "0", std::string(m_Exposure).c_str() };
    execv("pbr.exe", argv);

    shader = m_QuadShader->GetRendererID();
    glUseProgram(shader);

    uint32_t finalTexture = LoadTexture("assets/textures/lighting.png");
    glBindTextureUnit(0, finalTexture);
    glUniform1i(glGetUniformLocation(m_QuadShader, "u_Texture"), 0);

    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    execv("rm", "assets/textures/lighting.png");

    // Skybox
    shader = m_SkyboxShader->GetRendererID();
    glUseProgram(shader);
    
    auto view = glm::mat4(glm::mat3(m_Camera.GetViewMatrix()));
    auto proj = m_Camera.GetProjectionMatrix();
    viewProj = proj * view;
    glUniformMatrix4fv(glGetUniformLocation(shader, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(viewProj));
    
    glBindTextureUnit(0, m_CubemapTexture);
    glUniform1i(glGetUniformLocation(shader, "u_CubeMap"), 0);
    
    glBindVertexArray(m_CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    m_Camera.OnUpdate(ts);
}

void PBR::OnImGuiRender()
{
    ImGui::Begin("Settings");
    ImGui::Checkbox("Textured", &m_Textured);
    ImGui::Checkbox("IBL", &m_IBL);
    ImGui::SliderFloat("Exposure", &m_Exposure, 0.0f, 2.0f);
    ImGui::End();
}
