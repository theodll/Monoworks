#pragma once
#include <common/Base.hh>

namespace Monoworks::RHI 
{

	enum EShaderDataType : uint8_t
	{
		MW_SHADER_DATA_TYPE_NONE,
		MW_SHADER_DATA_TYPE_FLOAT,
		MW_SHADER_DATA_TYPE_FLOAT_2,
		MW_SHADER_DATA_TYPE_FLOAT_3,
		MW_SHADER_DATA_TYPE_FLOAT_4,
		MW_SHADER_DATA_TYPE_MAT_3,
		MW_SHADER_DATA_TYPE_MAT_4,
		MW_SHADER_DATA_TYPE_INT,
		MW_SHADER_DATA_TYPE_INT_2,
		MW_SHADER_DATA_TYPE_INT_3,
		MW_SHADER_DATA_TYPE_INT_4,
		MW_SHADER_DATA_TYPE_BOOL,

		MW_SHADER_DATA_TYPE_COUNT
	};

	static u32 ShaderDataTypeSize(EShaderDataType type)
	{
		switch (type)
		{
		case MW_SHADER_DATA_TYPE_FLOAT:		return 4;
		case MW_SHADER_DATA_TYPE_FLOAT_2:   return 4 * 2;
		case MW_SHADER_DATA_TYPE_FLOAT_3:   return 4 * 3;
		case MW_SHADER_DATA_TYPE_FLOAT_4:   return 4 * 4;
		case MW_SHADER_DATA_TYPE_MAT_3:     return 4 * 3 * 3; // 9 Floats
		case MW_SHADER_DATA_TYPE_MAT_4:     return 4 * 4 * 4; // 16 Floats
		case MW_SHADER_DATA_TYPE_INT:		return 4;
		case MW_SHADER_DATA_TYPE_INT_2:     return 4 * 2;
		case MW_SHADER_DATA_TYPE_INT_3:     return 4 * 3;
		case MW_SHADER_DATA_TYPE_INT_4:     return 4 * 4;
		case MW_SHADER_DATA_TYPE_BOOL:		return 4;
		case MW_SHADER_DATA_TYPE_NONE:		return 0;
		}
		MW_ASSERT(false, "Unknown ShaderDataType");
		return 0;
	}

	struct SBufferElement
	{
		std::string Name;
		EShaderDataType Type;
		u32 Size;
		u32 Count;
		u32 Offset{};

		SBufferElement(ShaderDataType type, const std::string& name)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)) {
		};

		u32 GetComponentCount() const
		{
			switch (Type)
			{
			case MW_SHADER_DATA_TYPE_FLOAT:		return 1;
			case MW_SHADER_DATA_TYPE_FLOAT_2:   return 2;
			case MW_SHADER_DATA_TYPE_FLOAT_3:   return 3;
			case MW_SHADER_DATA_TYPE_FLOAT_4:   return 4;
			case MW_SHADER_DATA_TYPE_MAT_3:     return 3 * 3;
			case MW_SHADER_DATA_TYPE_MAT_4:     return 4 * 4;
			case MW_SHADER_DATA_TYPE_INT:		return 1;
			case MW_SHADER_DATA_TYPE_INT_2:     return 2;
			case MW_SHADER_DATA_TYPE_INT_3:     return 3;
			case MW_SHADER_DATA_TYPE_INT_4:     return 4;
			case MW_SHADER_DATA_TYPE_BOOL:		return 1;
			case MW_SHADER_DATA_TYPE_NONE:		return 0;
			}

			return 0;
		}
	};

	class CBufferLayout
	{
	public:
		CBufferLayout(std::initializer_list<SBufferElement> element)
			: m_Elements(element)
		{
			CalculateOffsetAndStride();
		};

		CBufferLayout()
		{
			CalculateOffsetAndStride();
		};

		inline const std::vector<SBufferElement>& GetElements() const { return m_Elements; }
		u32 GetElementCount() const { return (u32)m_Elements.size(); }
		u64 GetStride() const { return m_Stride; }

		std::vector<SBufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<SBufferElement>::iterator end() { return m_Elements.end(); }

	private:
		void CalculateOffsetAndStride();
		std::vector<SBufferElement> m_Elements;
		u64 m_Stride;
	};

	typedef CBufferLayout VertexLayout;

	class IVertexBuffer 
	{
	public:
		IVertexBuffer() = delete;
		virtual ~IVertexBuffer() = default;

		virtual void SetData(void* data, u64 size, u64 offset = 0) = 0;
		virtual void SetLayout(const BufferLayout& layout) = 0;

		virtual void Upload(VkCommandBuffer commandBuffer) = 0;

		virtual CBufferLayout GetLayout() const = 0;
		virtual VkBuffer GetVulkanBuffer() const = 0;

		static Ref<IVertexBuffer> Create(void* vertexData, u64 vertexCount, u64 vertexStride, bool autoupload = false);

	};
}