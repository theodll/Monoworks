package org.monoworks.tools

import io.ktor.server.application.*
import io.ktor.server.response.*
import io.ktor.server.routing.*

fun Application.configureRouting() {
    routing {

       /* put("/crash-repotrs")
        {
            // load crash report zip into memory and check if it's a zip bomb
            // generate a UUID for the report and write it with the UUID as name to a specific place (local dir or S3 or smth)
            // extract the report.json and write the data into a database with the linked UUID
        }*/
    }
}