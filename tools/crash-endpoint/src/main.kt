package org.monoworks.tools

import kotlinx.serialization.Serializable

@Serializable
data class EndpointConfig( val reportDir: String? );

fun main(args: Array<String>) {
    io.ktor.server.netty.EngineMain.main(args)
}
