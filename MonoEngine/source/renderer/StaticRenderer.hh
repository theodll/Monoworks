#include <common/Base.hh>
#include <common/Memory.hh>
#include <common/Math.hh>

#include <rhi/GraphicsAPI.hh>

namespace Monoworks::RHI 
{
    class CStaticRenderer
    {
    public:
        static void Init() NOEXCEPT;
        static void Shutdown() NOEXCEPT;

    private:

        static Ref<RHI::IGraphicsAPI> m_Instance;
    };
}