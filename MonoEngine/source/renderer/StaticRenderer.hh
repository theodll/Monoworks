#include <common/Base.hh>
#include <common/Memory.hh>

#include <rhi/GraphicsAPI.hh>

namespace Monoworks::RHI 
{
    class CStaticRenderer 
    {
        public:
        static void Init() noexcept;
        static void Shutdown() noexcept 
        
        private:

        static Ref<RHI::IGraphicsAPI> m_Instance;
    }    
}