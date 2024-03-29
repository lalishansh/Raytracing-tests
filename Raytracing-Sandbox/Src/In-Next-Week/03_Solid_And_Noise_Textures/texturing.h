﻿
#include "../base.h"

namespace In_Next_Week
{
	// 15 floats + 9 floats = 24(multiple of 4)
	struct Material_03
	{
		float  RefractiveIndex;

		float Refractivity, Reflectivity;
		float  Scatteritivity[2];
		glm::vec3 Color;
		float TextureIndex; // NOTE: I'll cube-sphere cubic UV mapping for texturing instead of Mer-crate UV projection, as it can mapped to both cuboid and ellipsoid
	};
	struct GeometryBuff_03
		: public Transform_Buff // 18 float
	{
		float Type; // +1 float

		Material_03 MTL; // +9 float = 28 = 7*4 floats or 112 bytes
	};
	enum class GeometryType_03: int
	{
		None = 0,
		Ellipsoid,
		Cuboid
	};
	class GeometryData_03
		: public Transform_Data
	{
	public:
		GeometryData_03 (): Transform_Data () {};
		virtual ~GeometryData_03 () {
			for (auto &iVal : AllTextures) {
				glDeleteTextures (1, &iVal.first);
			}
			AllTextures.clear ();
		};

		// virtual std::pair<glm::vec3, glm::vec3> CalculateBBMinMax () override

		virtual bool OnImGuiRender () override
		{
			auto prev = TextureIndex;
			bool combo = ImGui::Combo ("Texture", &TextureIndex, AllTexturesStr.data ());
			if (combo) {
				// decrease counter
				if (prev) {
					LOG_ASSERT (AllTextures[prev - 1].second > 0);
					AllTextures[prev - 1].second--;
				}
				if (TextureIndex) {
					AllTextures[TextureIndex - 1].second++;
				}
			}
			return
				  combo
				| ImGui::SliderFloat2 ("Scatteritivity (OnRefract, OnReflect)", &Scatteritivity[0], 0.0f, 1.0f)
				| ImGui::SliderFloat ("Reflectivity", &Reflectivity, 0.0f, 1.0f - Refractivity)
				| ImGui::SliderFloat ("Refractivity", &Refractivity, 0.0f, 1.0f)
				| ImGui::SliderFloat ("Refractive Index", &RefractiveIndex, 1.0f, 3.0f)
				| ImGui::ColorEdit3 ("Color", &Color[0])
				| ImGui::Combo ("Geometry Type", (int *)&Type, "None\0Ellipsoid\0Cuboid\0")
				| OnImGuiRenderMin ();
		}

		void FillBuffer (GeometryBuff_03 *buffer)
		{
			FillTransformBuff (buffer);
			buffer->Type = float (Type);
			
			buffer->Type = float (Type);
			
			buffer->MTL.Color = Color;
			uint32_t counter = 0;
			for (uint32_t i = 0; i < AllTextures.size (); i++) {
				if (AllTextures[i].second > 0) counter++;
				if (TextureIndex == i) break;
			}
			buffer->MTL.TextureIndex = float(counter);

			buffer->MTL.RefractiveIndex = RefractiveIndex;
			buffer->MTL.Reflectivity = Reflectivity;
			buffer->MTL.Refractivity = Refractivity;

			buffer->MTL.Scatteritivity[0] = Scatteritivity[0], buffer->MTL.Scatteritivity[1] = Scatteritivity[1];
		}

		// if Load From Disk Use GLCore::Utils::FileDialogs::OpenFile("Image\0*.jpeg\0*.png\0*.bmp\0*.hdr\0*.psd\0*.tga\0*.gif\0*.pic\0*.psd\0*.pgm\0").c_str () as input
		static GLuint AddTextureOption (const char *filePath = nullptr, Helper::TEXTURE_2D::MAPPING loadAs = Helper::TEXTURE_2D::MAPPING::CUBIC)
		{
			std::optional<std::tuple<GLuint, uint32_t, uint32_t>> temp;
			if (loadAs != Helper::TEXTURE_2D::MAPPING::CUBIC)
				temp = Helper::TEXTURE_2D::LoadFromDiskToGPU (filePath, loadAs, Helper::TEXTURE_2D::MAPPING::CUBIC);
			else temp = Helper::TEXTURE_2D::LoadFromDiskToGPU (filePath);
			if (temp) {
				auto [a, b, c] = temp.value ();
				AllTextures.push_back ({ a, 0 });
				std::string name = std::filesystem::path (filePath).filename ().string ();
				AllTexturesStr.insert(AllTexturesStr.end() - 1, '\0');
				for (auto r_itr = name.begin (); r_itr != name.end (); r_itr++) {
					AllTexturesStr.insert (AllTexturesStr.end () - 2, *r_itr);
				}
				return a;
			}
			return 0;
		}
		static void AddTextureOption (GLuint textureID, const std::string &name)
		{
			LOG_ASSERT(textureID != 0 && name != std::string());
			{
				AllTextures.push_back ({ textureID, 0 });
				AllTexturesStr.insert (AllTexturesStr.end () - 1, '\0');
				for (auto r_itr = name.begin (); r_itr != name.end (); r_itr++) {
					AllTexturesStr.insert (AllTexturesStr.end () - 2, *r_itr);
				}
			}
		}
		static void BindTextureOption ()
		{
			std::vector<GLuint> send;
			for (auto& iVal : AllTextures)
				if (iVal.second > 0) send.push_back (iVal.first);

			uint32_t Size = MIN (send.size (), MaxTextureBindSlots);
			for (uint32_t i = 0; i < Size; i++) {
				glActiveTexture (GL_TEXTURE2 + i);
				glBindTexture (GL_TEXTURE_2D, send[i]);
			}
		}

		GeometryType_03 Type = GeometryType_03::Ellipsoid;
		glm::vec3 Color = { 0,0,0 };
		int TextureIndex = 0;
		float RefractiveIndex = 1.5f;
		float Refractivity = 0.65f;
		float Reflectivity = 0.15f;
		glm::vec2 Scatteritivity = { 0,0 }; // OnRefract, OnReflect

		// TextureID, counter
		static uint32_t MaxTextureBindSlots, OffsetBindSlots;
		static std::vector<std::pair<GLuint, uint32_t>> AllTextures;
		static std::vector<char> AllTexturesStr; // Split By '\0' to directly be used for ImGui::Combo
	};


	class Texturing: public RT_Base<GeometryData_03, GeometryBuff_03> // private
	{
	public:
		Texturing ()
			: RT_Base<GeometryData_03, GeometryBuff_03> ("In-Next-Week : Textures and Noise", "No discription", Helper::ReadFileAsString (".\\Src\\In-Next-Week\\03_Solid_And_Noise_Textures\\computeShaderSrc.glsl", '#').c_str ())
		{}
		virtual ~Texturing () = default;

		virtual void OnUpdate (GLCore::Timestep) override;
		virtual void OnAttach () override;
		virtual void OnDetach () override;
		virtual void OnEvent (GLCore::Event &event) override;
		virtual void OnImGuiRender () override;
		virtual void ImGuiMenuOptions () override {}

		virtual void OnComputeShaderReload () override;

		virtual bool FillBuffer (GLCore::Timestep) override;

		int m_MaxTextureBindSlots = 6;
	};
}