package org.monoworks.tools

import io.ktor.http.HttpStatusCode
import io.ktor.http.content.PartData
import io.ktor.http.content.forEachPart
import io.ktor.server.request.receiveMultipart
import io.ktor.server.response.respond
import io.ktor.server.routing.*
import io.ktor.utils.io.toByteArray
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.util.zip.ZipInputStream

import kotlin.uuid.*;

data class ReportLimits(
    val MaxZippedBytes: Long = 100L * 1024L * 1024L,
    val MaxTotalBytes: Long = 250L * 1024L * 1024L,
    val MaxZipEntries: Int = 100
)

fun ExtractReport( zip: ByteArray, reportLimits: ReportLimits ): ByteArray?
{
    require(zip.size <= reportLimits.MaxZippedBytes )
    {
        "Compressed report to large";
    }

    var entries = 0;

    ZipInputStream(ByteArrayInputStream(zip)).use { zipInputStream ->
        val buffer = ByteArray(zipInputStream.available());

        while (true)
        {
            val entry = zipInputStream.nextEntry ?: break;
            entries++;

            if (entry.name != "report.json" || entry.isDirectory )
            {
                zipInputStream.close();
                continue;
            }

            val output = ByteArrayOutputStream();
            var totalBytes = 0;

            while (true)
            {
                val read = zipInputStream.read(buffer);

                if (read == -1) break;

                totalBytes += read;

                require( totalBytes <= reportLimits.MaxZippedBytes )
                {
                    "report.json is too large";
                }

                output.write(buffer, 0, read);

            }

            zipInputStream.close();
            return output.toByteArray();

        }

    }

    return null;

};


fun Route.ReceiveReport( )
{
    post("/upload")
    {
        val multipartDat  = call.receiveMultipart();
        var report: ByteArray? = null;

        multipartDat.forEachPart { part ->
            if (part is PartData.FileItem)
            {
                report = part.provider().toByteArray()
            };
            part.release()
        }

        if (report == null)
        {
            call.respond(HttpStatusCode.BadRequest)
        }

        val json = report?.decodeToString();


    }

}