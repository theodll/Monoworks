#include <Monoworks.hh>
#include "QOpenGLExternalObjectsExtraFunctions.hh"
#include <QtCore/QDebug>

namespace Monoworks 
{

#define GL_EXT_CALL_VOID(ptr, ...) \
    do { if ((ptr) != nullptr) { (ptr)(__VA_ARGS__); } else { MW_WARN( " OpenGL Function at adress {} not loaded", ptr ); } } while (false)

    QOpenGLExternalObjectsExtraFunctions::QOpenGLExternalObjectsExtraFunctions( QOpenGLContext* pContext )
    {
        if ( pContext != nullptr && pContext == QOpenGLContext::currentContext() ) {
            initializeExternalObjectsFunctions();
        }
    }

    bool QOpenGLExternalObjectsExtraFunctions::initializeExternalObjectsFunctions()
    {
        if ( m_Initialized ) {
            return true;
        }

        QOpenGLContext* pContext = QOpenGLContext::currentContext();
        if ( pContext == nullptr ) {
            return false;
        }

        initializeOpenGLFunctions();

        m_HasMemoryObjectEXT = pContext->hasExtension( QByteArrayLiteral( "GL_EXT_memory_object" ) );
        m_HasSemaphoreEXT = pContext->hasExtension( QByteArrayLiteral( "GL_EXT_semaphore" ) );
#ifdef MW_PLATFORM_WINDOWS
        m_HasMemoryObjectWin32EXT = pContext->hasExtension( QByteArrayLiteral( "GL_EXT_memory_object_win32" ) );
        m_HasSemaphoreWin32EXT = pContext->hasExtension( QByteArrayLiteral( "GL_EXT_semaphore_win32" ) );
#else
        m_HasMemoryObjectFdEXT = pContext->hasExtension( QByteArrayLiteral( "GL_EXT_memory_object_fd" ) );
        m_HasSemaphoreFdEXT = pContext->hasExtension( QByteArrayLiteral( "GL_EXT_semaphore_fd" ) );
#endif
        if ( m_HasMemoryObjectEXT || m_HasSemaphoreEXT ) {
            m_pGlGetUnsignedBytevEXT = resolve<PfnGlGetUnsignedBytevEXTProc>( "glGetUnsignedBytevEXT" );
            m_pGlGetUnsignedByteiVEXT = resolve<PfnGlGetUnsignedByteiVEXTProc>( "glGetUnsignedBytei_vEXT" );
        }

        if ( m_HasMemoryObjectEXT ) {
            m_pGlDeleteMemoryObjectsEXT = resolve<PfnGlDeleteMemoryObjectsEXTProc>( "glDeleteMemoryObjectsEXT" );
            m_pGlIsMemoryObjectEXT = resolve<PfnGlIsMemoryObjectEXTProc>( "glIsMemoryObjectEXT" );
            m_pGlCreateMemoryObjectsEXT = resolve<PfnGlCreateMemoryObjectsEXTProc>( "glCreateMemoryObjectsEXT" );
            m_pGlMemoryObjectParameterivEXT = resolve<PfnGlMemoryObjectParameterivEXTProc>( "glMemoryObjectParameterivEXT" );
            m_pGlGetMemoryObjectParameterivEXT = resolve<PfnGlGetMemoryObjectParameterivEXTProc>( "glGetMemoryObjectParameterivEXT" );

            m_pGlTexStorageMem1DEXT = resolve<PfnGlTexStorageMem1DEXTProc>( "glTexStorageMem1DEXT" );
            m_pGlTexStorageMem2DEXT = resolve<PfnGlTexStorageMem2DEXTProc>( "glTexStorageMem2DEXT" );
            m_pGlTexStorageMem2DMultisampleEXT = resolve<PfnGlTexStorageMem2DMultisampleEXTProc>( "glTexStorageMem2DMultisampleEXT" );
            m_pGlTexStorageMem3DEXT = resolve<PfnGlTexStorageMem3DEXTProc>( "glTexStorageMem3DEXT" );
            m_pGlTexStorageMem3DMultisampleEXT = resolve<PfnGlTexStorageMem3DMultisampleEXTProc>( "glTexStorageMem3DMultisampleEXT" );

            m_pGlTextureStorageMem1DEXT = resolve<PfnGlTextureStorageMem1DEXTProc>( "glTextureStorageMem1DEXT" );
            m_pGlTextureStorageMem2DEXT = resolve<PfnGlTextureStorageMem2DEXTProc>( "glTextureStorageMem2DEXT" );
            m_pGlTextureStorageMem2DMultisampleEXT = resolve<PfnGlTextureStorageMem2DMultisampleEXTProc>( "glTextureStorageMem2DMultisampleEXT" );
            m_pGlTextureStorageMem3DEXT = resolve<PfnGlTextureStorageMem3DEXTProc>( "glTextureStorageMem3DEXT" );
            m_pGlTextureStorageMem3DMultisampleEXT = resolve<PfnGlTextureStorageMem3DMultisampleEXTProc>( "glTextureStorageMem3DMultisampleEXT" );

            m_pGlBufferStorageMemEXT = resolve<PfnGlBufferStorageMemEXTProc>( "glBufferStorageMemEXT" );
            m_pGlNamedBufferStorageMemEXT = resolve<PfnGlNamedBufferStorageMemEXTProc>( "glNamedBufferStorageMemEXT" );
        }

        if ( m_HasSemaphoreEXT ) {
            m_pGlGenSemaphoresEXT = resolve<PfnGlGenSemaphoresEXTProc>( "glGenSemaphoresEXT" );
            m_pGlDeleteSemaphoresEXT = resolve<PfnGlDeleteSemaphoresEXTProc>( "glDeleteSemaphoresEXT" );
            m_pGlIsSemaphoreEXT = resolve<PfnGlIsSemaphoreEXTProc>( "glIsSemaphoreEXT" );
            m_pGlSemaphoreParameterui64vEXT = resolve<PfnGlSemaphoreParameterui64vEXTProc>( "glSemaphoreParameterui64vEXT" );
            m_pGlGetSemaphoreParameterui64vEXT = resolve<PfnGlGetSemaphoreParameterui64vEXTProc>( "glGetSemaphoreParameterui64vEXT" );
            m_pGlWaitSemaphoreEXT = resolve<PfnGlWaitSemaphoreEXTProc>( "glWaitSemaphoreEXT" );
            m_pGlSignalSemaphoreEXT = resolve<PfnGlSignalSemaphoreEXTProc>( "glSignalSemaphoreEXT" );
        }

        if ( m_HasMemoryObjectFdEXT ) {
            m_pGlImportMemoryFdEXT = resolve<PfnGlImportMemoryFdEXTProc>( "glImportMemoryFdEXT" );
        }

        if ( m_HasSemaphoreFdEXT ) {
            m_pGlImportSemaphoreFdEXT = resolve<PfnGlImportSemaphoreFdEXTProc>( "glImportSemaphoreFdEXT" );
        }

        if ( m_HasMemoryObjectWin32EXT ) {
            m_pGlImportMemoryWin32HandleEXT = resolve<PfnGlImportMemoryWin32HandleEXTProc>( "glImportMemoryWin32HandleEXT" );
            m_pGlImportMemoryWin32NameEXT = resolve<PfnGlImportMemoryWin32NameEXTProc>( "glImportMemoryWin32NameEXT" );
        }

        if ( m_HasSemaphoreWin32EXT ) {
            m_pGlImportSemaphoreWin32HandleEXT = resolve<PfnGlImportSemaphoreWin32HandleEXTProc>( "glImportSemaphoreWin32HandleEXT" );
            m_pGlImportSemaphoreWin32NameEXT = resolve<PfnGlImportSemaphoreWin32NameEXTProc>( "glImportSemaphoreWin32NameEXT" );
        }

        m_Initialized = true;
        return true;
    }

    void QOpenGLExternalObjectsExtraFunctions::glGetUnsignedBytevEXT( GLenum pname, GLubyte* pData )
    {
        GL_EXT_CALL_VOID( m_pGlGetUnsignedBytevEXT, pname, pData );
    }

    void QOpenGLExternalObjectsExtraFunctions::glGetUnsignedBytei_vEXT( GLenum target, GLuint index, GLubyte* pData )
    {
        GL_EXT_CALL_VOID( m_pGlGetUnsignedByteiVEXT, target, index, pData );
    }

    void QOpenGLExternalObjectsExtraFunctions::glDeleteMemoryObjectsEXT( GLsizei n, const GLuint* pMemoryObjects )
    {
        GL_EXT_CALL_VOID( m_pGlDeleteMemoryObjectsEXT, n, pMemoryObjects );
    }

    GLboolean QOpenGLExternalObjectsExtraFunctions::glIsMemoryObjectEXT( GLuint memoryObject )
    {
        return (m_pGlIsMemoryObjectEXT != nullptr) ? m_pGlIsMemoryObjectEXT( memoryObject ) : GL_FALSE;
    }

    void QOpenGLExternalObjectsExtraFunctions::glCreateMemoryObjectsEXT( GLsizei n, GLuint* pMemoryObjects )
    {
        GL_EXT_CALL_VOID( m_pGlCreateMemoryObjectsEXT, n, pMemoryObjects );
    }

    void QOpenGLExternalObjectsExtraFunctions::glMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, const GLint* pParams )
    {
        GL_EXT_CALL_VOID( m_pGlMemoryObjectParameterivEXT, memoryObject, pname, pParams );
    }

    void QOpenGLExternalObjectsExtraFunctions::glGetMemoryObjectParameterivEXT( GLuint memoryObject, GLenum pname, GLint* pParams )
    {
        GL_EXT_CALL_VOID( m_pGlGetMemoryObjectParameterivEXT, memoryObject, pname, pParams );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTexStorageMem1DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTexStorageMem1DEXT, target, levels, internalFormat, width, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTexStorageMem2DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTexStorageMem2DEXT, target, levels, internalFormat, width, height, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTexStorageMem2DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTexStorageMem2DMultisampleEXT, target, samples, internalFormat, width, height, fixedSampleLocations, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTexStorageMem3DEXT( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTexStorageMem3DEXT, target, levels, internalFormat, width, height, depth, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTexStorageMem3DMultisampleEXT( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTexStorageMem3DMultisampleEXT, target, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTextureStorageMem1DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTextureStorageMem1DEXT, texture, levels, internalFormat, width, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTextureStorageMem2DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTextureStorageMem2DEXT, texture, levels, internalFormat, width, height, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTextureStorageMem2DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTextureStorageMem2DMultisampleEXT, texture, samples, internalFormat, width, height, fixedSampleLocations, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTextureStorageMem3DEXT( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTextureStorageMem3DEXT, texture, levels, internalFormat, width, height, depth, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glTextureStorageMem3DMultisampleEXT( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlTextureStorageMem3DMultisampleEXT, texture, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glBufferStorageMemEXT( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlBufferStorageMemEXT, target, size, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glNamedBufferStorageMemEXT( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset )
    {
        GL_EXT_CALL_VOID( m_pGlNamedBufferStorageMemEXT, buffer, size, memory, offset );
    }

    void QOpenGLExternalObjectsExtraFunctions::glGenSemaphoresEXT( GLsizei n, GLuint* pSemaphores )
    {
        GL_EXT_CALL_VOID( m_pGlGenSemaphoresEXT, n, pSemaphores );
    }

    void QOpenGLExternalObjectsExtraFunctions::glDeleteSemaphoresEXT( GLsizei n, const GLuint* pSemaphores )
    {
        GL_EXT_CALL_VOID( m_pGlDeleteSemaphoresEXT, n, pSemaphores );
    }

    GLboolean QOpenGLExternalObjectsExtraFunctions::glIsSemaphoreEXT( GLuint semaphore )
    {
        return (m_pGlIsSemaphoreEXT != nullptr) ? m_pGlIsSemaphoreEXT( semaphore ) : GL_FALSE;
    }

    void QOpenGLExternalObjectsExtraFunctions::glSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, const GLuint64* pParams )
    {
        GL_EXT_CALL_VOID( m_pGlSemaphoreParameterui64vEXT, semaphore, pname, pParams );
    }

    void QOpenGLExternalObjectsExtraFunctions::glGetSemaphoreParameterui64vEXT( GLuint semaphore, GLenum pname, GLuint64* pParams )
    {
        GL_EXT_CALL_VOID( m_pGlGetSemaphoreParameterui64vEXT, semaphore, pname, pParams );
    }

    void QOpenGLExternalObjectsExtraFunctions::glWaitSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint* pBuffers, GLuint numTextureBarriers, const GLuint* pTextures, const GLenum* pSrcLayouts )
    {
        GL_EXT_CALL_VOID( m_pGlWaitSemaphoreEXT, semaphore, numBufferBarriers, pBuffers, numTextureBarriers, pTextures, pSrcLayouts );
    }

    void QOpenGLExternalObjectsExtraFunctions::glSignalSemaphoreEXT( GLuint semaphore, GLuint numBufferBarriers, const GLuint* pBuffers, GLuint numTextureBarriers, const GLuint* pTextures, const GLenum* pDstLayouts )
    {
        GL_EXT_CALL_VOID( m_pGlSignalSemaphoreEXT, semaphore, numBufferBarriers, pBuffers, numTextureBarriers, pTextures, pDstLayouts );
    }

    void QOpenGLExternalObjectsExtraFunctions::glImportMemoryFdEXT( GLuint memory, GLuint64 size, GLenum handleType, GLint fd )
    {
        GL_EXT_CALL_VOID( m_pGlImportMemoryFdEXT, memory, size, handleType, fd );
    }

    void QOpenGLExternalObjectsExtraFunctions::glImportSemaphoreFdEXT( GLuint semaphore, GLenum handleType, GLint fd )
    {
        GL_EXT_CALL_VOID( m_pGlImportSemaphoreFdEXT, semaphore, handleType, fd );
    }

    void QOpenGLExternalObjectsExtraFunctions::glImportMemoryWin32HandleEXT( GLuint memory, GLuint64 size, GLenum handleType, void* pHandle )
    {
        GL_EXT_CALL_VOID( m_pGlImportMemoryWin32HandleEXT, memory, size, handleType, pHandle );
    }

    void QOpenGLExternalObjectsExtraFunctions::glImportMemoryWin32NameEXT( GLuint memory, GLuint64 size, GLenum handleType, const void* pName )
    {
        GL_EXT_CALL_VOID( m_pGlImportMemoryWin32NameEXT, memory, size, handleType, pName );
    }

    void QOpenGLExternalObjectsExtraFunctions::glImportSemaphoreWin32HandleEXT( GLuint semaphore, GLenum handleType, void* pHandle )
    {
        GL_EXT_CALL_VOID( m_pGlImportSemaphoreWin32HandleEXT, semaphore, handleType, pHandle );
    }

    void QOpenGLExternalObjectsExtraFunctions::glImportSemaphoreWin32NameEXT( GLuint semaphore, GLenum handleType, const void* pName )
    {
        GL_EXT_CALL_VOID( m_pGlImportSemaphoreWin32NameEXT, semaphore, handleType, pName );
    }

#undef GL_EXT_CALL_VOID
}