#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

using namespace GLCore;
using namespace GLCore::Utils;

class PBR : public Layer
{
public:
	PBR();
	virtual ~PBR();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& e) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;

private:
	Camera m_Camera;

	Shader* m_BRDFIntegrationShader;
	Shader* m_EquirectangularToCubemapShader;
	Shader* m_IrradianceShader;
	Shader* m_PBRShader;
	Shader* m_PrefilterShader;
	Shader* m_SkyboxShader;
	Shader* m_QuadShader;

	uint32_t m_EnvironmentFBO;
	uint32_t m_CubemapTexture;
	uint32_t m_CubemapDepthRBO;

	uint32_t m_BRDFLUT;
	uint32_t m_IrradianceTexture;
	uint32_t m_PrefilteredEnvMap;

	uint32_t m_CubeVAO;
	uint32_t m_QuadVAO;

	uint32_t m_SphereVAO;
	uint32_t m_SphereIBO;
	uint32_t m_SphereIndexCount;

	uint32_t m_SphereAlbedoMap;
	uint32_t m_SphereAOMap;
	uint32_t m_SphereMetallicMap;
	uint32_t m_SphereNormalMap;
	uint32_t m_SphereRoughnessMap;
	uint32_t m_SphereHeightMap;

	bool m_Textured = true;
	float m_Exposure = 0.5f;
	bool m_IBL = true;

	void EquirectangularToCubemap(uint32_t equirectangularMap);
	void GenerateBRDFIntegration(uint32_t environment);
	void GenerateIrradiance(uint32_t environment);
	void GeneratePrefilteredEnvMap(uint32_t environment);
};
