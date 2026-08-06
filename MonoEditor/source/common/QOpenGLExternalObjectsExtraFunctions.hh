// Implemented after the specification at: 
// https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects.txt
// https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects_fd.txt
// https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects_win32.txt
#pragma once

#include <QtGui/QOpenGLExtraFunctions>
#include <QtGui/QOpenGLContext>
#include <QOpenGLFunctions_4_5_Core>
#include <QtCore/QByteArray>
#include <QtCore/QtGlobal>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif
#include <GL/gl.h>

#ifndef APIENTRY
#  define APIENTRY
#endif
#ifndef APIENTRYP
#  define APIENTRYP APIENTRY *
#endif
#ifndef GL_APIENTRYP
#  define GL_APIENTRYP APIENTRYP
#endif
namespace Monoworks
{
    static constexpr GLenum kGlTextureTilingEXT = 0x9580;
    static constexpr GLenum kGlDedicatedMemoryObjectEXT = 0x9581;
    static constexpr GLenum kGlNumTilingTypesEXT = 0x9582;
    static constexpr GLenum kGlTilingTypesEXT = 0x9583;
    static constexpr GLenum kGlOptimalTilingEXT = 0x9584;
    static constexpr GLenum kGlLinearTilingEXT = 0x9585;
    static constexpr GLenum kGlHandleTypeOpaqueFdEXT = 0x9586;
    static constexpr GLenum kGlLayoutGeneralEXT = 0x958D;
    static constexpr GLenum kGlLayoutColorAttachmentEXT = 0x958E;
    static constexpr GLenum kGlLayoutDepthStencilAttachmentEXT = 0x958F;
    static constexpr GLenum kGlLayoutDepthStencilReadOnlyEXT = 0x9590;
    static constexpr GLenum kGlLayoutShaderReadOnlyEXT = 0x9591;
    static constexpr GLenum kGlLayoutTransferSrcEXT = 0x9592;
    static constexpr GLenum kGlLayoutTransferDstEXT = 0x9593;
    static constexpr GLenum kGlLayoutDepthReadOnlyStencilAttachmentEXT = 0x9530;
    static constexpr GLenum kGlLayoutDepthAttachmentStencilReadOnlyEXT = 0x9531;
    static constexpr GLenum kGlNumDeviceUUIDsEXT = 0x9596;
    static constexpr GLenum kGlDeviceUUIDEXT = 0x9597;
    static constexpr GLenum kGlDriverUUIDEXT = 0x9598;
    static constexpr GLenum kGlProtectedMemoryObjectEXT = 0x959B;
    static constexpr GLint  kGlUUIDSizeEXT = 16;

    // GL_EXT_memory_objects_win32
    static constexpr GLenum kGlHandleTypeOpaqueWin32EXT = 0x9587;
    static constexpr GLenum kGlHandleTypeOpaqueWin32KmtEXT = 0x9588;
    static constexpr GLenum kGlHandleTypeD3D12TilepoolEXT = 0x9589;
    static constexpr GLenum kGlHandleTypeD3D12ResourceEXT = 0x958A;
    static constexpr GLenum kGlHandleTypeD3D11ImageEXT = 0x958B;
    static constexpr GLenum kGlHandleTypeD3D11ImageKmtEXT = 0x958C;
    static constexpr GLenum kGlHandleTypeD3D12FenceEXT = 0x9594;
    static constexpr GLenum kGlD3D12FenceValueEXT = 0x9595;
    static constexpr GLenum kGlDeviceLuidEXT = 0x9599;
    static constexpr GLenum kGlDeviceNodeMaskEXT = 0x959A;
    static constexpr GLint  kGlLuidSizeEXT = 8;

    // GL_EXT_memory_ojects_fd
    static constexpr GLenum  kGlHandleTypeOpaqueFdEXT = 0x9586;
    // TODO: rename this class 
    class QOpenGLExternalObjectsExtraFunctions : public QOpenGLFunctions_4_5_Core
    {
    public:
        QOpenGLExternalObjectsExtraFunctions() = default;
        explicit QOpenGLExternalObjectsExtraFunctions( QOpenGLContext* pContext );

        [[nodiscard]] bool initializeExternalObjectsFunctions();
        [[nodiscard]] bool isInitialized() const noexcept { return m_Initialized; }

        [[nodiscard]] bool hasMemoryObjectEXT() const noexcept { return m_HasMemoryObjectEXT; }
        [[nodiscard]] bool hasSemaphoreEXT() const noexcept { return m_HasSemaphoreEXT; }
        [[nodiscard]] bool hasMemoryObjectFdEXT() const noexcept { return m_HasMemoryObjectFdEXT; }
        [[nodiscard]] bool hasSemaphoreFdEXT() const noexcept { return m_HasSemaphoreFdEXT; }

        [[nodiscard]] bool hasMemoryObjectWin32EXT() const noexcept { return m_HasMemoryObjectWin32EXT; }
        [[nodiscard]] bool hasSemaphoreWin32EXT() const noexcept { return m_HasSemaphoreWin32EXT; }

        // common
        void glGetUnsignedBytevEXT( GLenum pname, GLubyte* pData );
        void glGetUnsignedBytei_vEXT( GLenum target, GLuint index, GLubyte* pData );

        // GL_EXT_memory_object
        void glDeleteMemoryObjectsEXT( GLsizei n, const GLuint* pMemoryObjects );
        GLboolean glIsMemoryObjectEXT( GLuint memoryObject );
        void glCreateMemoryObjectsEXT( GLsizei n, GLuint* pMemoryObjects );
        void glMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, const GLint* pParams );
        void glGetMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, GLint* pParams );

        void glTexStorageMem1DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
        void glTexStorageMem2DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
        void glTexStorageMem2DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
        void glTexStorageMem3DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
        void glTexStorageMem3DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );

        void glTextureStorageMem1DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
        void glTextureStorageMem2DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
        void glTextureStorageMem2DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
        void glTextureStorageMem3DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
        void glTextureStorageMem3DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );

        void glBufferStorageMemEXT( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset );
        void glNamedBufferStorageMemEXT( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset );

        // GL_EXT_semaphores
        void glGenSemaphoresEXT( GLsizei n, GLuint* pSemaphores );
        void glDeleteSemaphoresEXT( GLsizei n, const GLuint* pSemaphores );
        GLboolean glIsSemaphoreEXT( GLuint semaphore );
        void glSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, const GLuint64* pParams );
        void glGetSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, GLuint64* pParams );
        void glWaitSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint* pBuffers, GLuint numTextureBarriers, const GLuint* pTextures, const GLenum* pSrcLayouts );
        void glSignalSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint* pBuffers, GLuint numTextureBarriers, const GLuint* pTextures, const GLenum* pDstLayouts );

        // GL_EXT_memory_object_fd
        void glImportMemoryFdEXT( GLuint memory, GLuint64 size, GLenum handleType, GLint fd );
        void glImportSemaphoreFdEXT( GLuint semaphore, GLenum handleType, GLint fd );

        // GL_EXT_memory_object_win32
        void glImportMemoryWin32HandleEXT( GLuint memory, GLuint64 size, GLenum handleType, void* pHandle );
        void glImportMemoryWin32NameEXT( GLuint memory, GLuint64 size, GLenum handleType, const void* pName );

        // GL_EXT_semaphore_win32
        void glImportSemaphoreWin32HandleEXT( GLuint semaphore, GLenum handleType, void* pHandle );
        void glImportSemaphoreWin32NameEXT( GLuint semaphore, GLenum handleType, const void* pName );

    private:
        template<typename T>
        [[nodiscard]] static T resolve( const char* pName )
        {
            QOpenGLContext* pContext = QOpenGLContext::currentContext();
            return pContext ? reinterpret_cast< T >(pContext->getProcAddress( QByteArray( pName ) )) : nullptr;
        }

        bool m_Initialized { false };
        
        // GL_EXT_memory_object
        bool m_HasMemoryObjectEXT { false };
        // GL_EXT_semaphore
        bool m_HasSemaphoreEXT { false };
        // GL_EXT_memory_object_fd
        bool m_HasMemoryObjectFdEXT { false };
        // GL_EXT_semaphore_fd
        bool m_HasSemaphoreFdEXT { false };
        // GL_EXT_memory_object_win32
        bool m_HasMemoryObjectWin32EXT { false };
        // GL_EXT_semaphore_win32
        bool m_HasSemaphoreWin32EXT { false };

        using PfnGlGetUnsignedBytevEXTProc = void (GL_APIENTRYP)( GLenum, GLubyte* );
        using PfnGlGetUnsignedByteiVEXTProc = void (GL_APIENTRYP)( GLenum, GLuint, GLubyte* );

        using PfnGlDeleteMemoryObjectsEXTProc = void (GL_APIENTRYP)( GLsizei, const GLuint* );
        using PfnGlIsMemoryObjectEXTProc = GLboolean( GL_APIENTRYP )(GLuint);
        using PfnGlCreateMemoryObjectsEXTProc = void (GL_APIENTRYP)( GLsizei, GLuint* );
        using PfnGlMemoryObjectParameterivEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, const GLint* );
        using PfnGlGetMemoryObjectParameterivEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, GLint* );

        using PfnGlTexStorageMem1DEXTProc = void (GL_APIENTRYP)( GLenum, GLsizei, GLenum, GLsizei, GLuint, GLuint64 );
        using PfnGlTexStorageMem2DEXTProc = void (GL_APIENTRYP)( GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLuint, GLuint64 );
        using PfnGlTexStorageMem2DMultisampleEXTProc = void (GL_APIENTRYP)( GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean, GLuint, GLuint64 );
        using PfnGlTexStorageMem3DEXTProc = void (GL_APIENTRYP)( GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLuint, GLuint64 );
        using PfnGlTexStorageMem3DMultisampleEXTProc = void (GL_APIENTRYP)( GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean, GLuint, GLuint64 );

        using PfnGlTextureStorageMem1DEXTProc = void (GL_APIENTRYP)( GLuint, GLsizei, GLenum, GLsizei, GLuint, GLuint64 );
        using PfnGlTextureStorageMem2DEXTProc = void (GL_APIENTRYP)( GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLuint, GLuint64 );
        using PfnGlTextureStorageMem2DMultisampleEXTProc = void (GL_APIENTRYP)( GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLboolean, GLuint, GLuint64 );
        using PfnGlTextureStorageMem3DEXTProc = void (GL_APIENTRYP)( GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLuint, GLuint64 );
        using PfnGlTextureStorageMem3DMultisampleEXTProc = void (GL_APIENTRYP)( GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean, GLuint, GLuint64 );

        using PfnGlBufferStorageMemEXTProc = void (GL_APIENTRYP)( GLenum, GLsizeiptr, GLuint, GLuint64 );
        using PfnGlNamedBufferStorageMemEXTProc = void (GL_APIENTRYP)( GLuint, GLsizeiptr, GLuint, GLuint64 );

        using PfnGlGenSemaphoresEXTProc = void (GL_APIENTRYP)( GLsizei, GLuint* );
        using PfnGlDeleteSemaphoresEXTProc = void (GL_APIENTRYP)( GLsizei, const GLuint* );
        using PfnGlIsSemaphoreEXTProc = GLboolean( GL_APIENTRYP )(GLuint);
        using PfnGlSemaphoreParameterui64vEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, const GLuint64* );
        using PfnGlGetSemaphoreParameterui64vEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, GLuint64* );
        using PfnGlWaitSemaphoreEXTProc = void (GL_APIENTRYP)( GLuint, GLuint, const GLuint*, GLuint, const GLuint*, const GLenum* );
        using PfnGlSignalSemaphoreEXTProc = void (GL_APIENTRYP)( GLuint, GLuint, const GLuint*, GLuint, const GLuint*, const GLenum* );

        using PfnGlImportMemoryFdEXTProc = void (GL_APIENTRYP)( GLuint, GLuint64, GLenum, GLint );
        using PfnGlImportSemaphoreFdEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, GLint );

        using PfnGlImportMemoryWin32HandleEXTProc = void (GL_APIENTRYP)( GLuint, GLuint64, GLenum, void* );
        using PfnGlImportMemoryWin32NameEXTProc = void (GL_APIENTRYP)( GLuint, GLuint64, GLenum, const void* );
        using PfnGlImportSemaphoreWin32HandleEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, void* );
        using PfnGlImportSemaphoreWin32NameEXTProc = void (GL_APIENTRYP)( GLuint, GLenum, const void* );

        PfnGlGetUnsignedBytevEXTProc m_pGlGetUnsignedBytevEXT { nullptr };
        PfnGlGetUnsignedByteiVEXTProc m_pGlGetUnsignedByteiVEXT { nullptr };

        PfnGlDeleteMemoryObjectsEXTProc m_pGlDeleteMemoryObjectsEXT { nullptr };
        PfnGlIsMemoryObjectEXTProc m_pGlIsMemoryObjectEXT { nullptr };
        PfnGlCreateMemoryObjectsEXTProc m_pGlCreateMemoryObjectsEXT { nullptr };
        PfnGlMemoryObjectParameterivEXTProc m_pGlMemoryObjectParameterivEXT { nullptr };
        PfnGlGetMemoryObjectParameterivEXTProc m_pGlGetMemoryObjectParameterivEXT { nullptr };

        PfnGlTexStorageMem1DEXTProc m_pGlTexStorageMem1DEXT { nullptr };
        PfnGlTexStorageMem2DEXTProc m_pGlTexStorageMem2DEXT { nullptr };
        PfnGlTexStorageMem2DMultisampleEXTProc m_pGlTexStorageMem2DMultisampleEXT { nullptr };
        PfnGlTexStorageMem3DEXTProc m_pGlTexStorageMem3DEXT { nullptr };
        PfnGlTexStorageMem3DMultisampleEXTProc m_pGlTexStorageMem3DMultisampleEXT { nullptr };

        PfnGlTextureStorageMem1DEXTProc m_pGlTextureStorageMem1DEXT { nullptr };
        PfnGlTextureStorageMem2DEXTProc m_pGlTextureStorageMem2DEXT { nullptr };
        PfnGlTextureStorageMem2DMultisampleEXTProc m_pGlTextureStorageMem2DMultisampleEXT { nullptr };
        PfnGlTextureStorageMem3DEXTProc m_pGlTextureStorageMem3DEXT { nullptr };
        PfnGlTextureStorageMem3DMultisampleEXTProc m_pGlTextureStorageMem3DMultisampleEXT { nullptr };

        PfnGlBufferStorageMemEXTProc m_pGlBufferStorageMemEXT { nullptr };
        PfnGlNamedBufferStorageMemEXTProc m_pGlNamedBufferStorageMemEXT { nullptr };

        PfnGlGenSemaphoresEXTProc m_pGlGenSemaphoresEXT { nullptr };
        PfnGlDeleteSemaphoresEXTProc m_pGlDeleteSemaphoresEXT { nullptr };
        PfnGlIsSemaphoreEXTProc m_pGlIsSemaphoreEXT { nullptr };
        PfnGlSemaphoreParameterui64vEXTProc m_pGlSemaphoreParameterui64vEXT { nullptr };
        PfnGlGetSemaphoreParameterui64vEXTProc m_pGlGetSemaphoreParameterui64vEXT { nullptr };
        PfnGlWaitSemaphoreEXTProc m_pGlWaitSemaphoreEXT { nullptr };
        PfnGlSignalSemaphoreEXTProc m_pGlSignalSemaphoreEXT { nullptr };

        // GL_EXT_memory_objects_fd
        PfnGlImportMemoryFdEXTProc m_pGlImportMemoryFdEXT { nullptr };
        PfnGlImportSemaphoreFdEXTProc m_pGlImportSemaphoreFdEXT { nullptr };

        // GL_EXT_memory_objects_win32
        PfnGlImportMemoryWin32HandleEXTProc m_pGlImportMemoryWin32HandleEXT { nullptr };
        PfnGlImportMemoryWin32NameEXTProc m_pGlImportMemoryWin32NameEXT { nullptr };
        PfnGlImportSemaphoreWin32HandleEXTProc m_pGlImportSemaphoreWin32HandleEXT { nullptr };
        PfnGlImportSemaphoreWin32NameEXTProc m_pGlImportSemaphoreWin32NameEXT { nullptr };
    };

}