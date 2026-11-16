package org.monoworks.tools.routes

import io.ktor.http.HttpStatusCode
import io.ktor.http.content.PartData
import io.ktor.http.content.forEachPart
import io.ktor.server.request.receiveMultipart
import io.ktor.server.response.respond
import io.ktor.server.routing.*
import io.ktor.utils.io.toByteArray
import kotlinx.io.files.Path
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.util.zip.ZipInputStream

data class ReportLimits(
    val MaxZippedBytes: Long = 100L * 1024L * 1024L,
    val MaxTotalBytes: Long = 250L * 1024L * 1024L,
    val MaxZipEntries: Int = 100
)

class CrashReport ( report: ByteArray, reportLimits: ReportLimits = ReportLimits() )
{
    private var m_ReportJSON: String? = null;
    private var m_ReportLimits: ReportLimits = reportLimits;
    private var m_ZippedReport: ByteArray = report;

    init
    {
        val JSONBytes = this.ExtractJSON();

    }

    public fun Store( dirPath: Path )
    {

    }

    private fun ExtractJSON(): ByteArray?
    {
        require(m_ZippedReport.size <= m_ReportLimits.MaxZippedBytes )
        {
            "Compressed report to large";
        }

        var entries = 0;

        ZipInputStream(ByteArrayInputStream(m_ZippedReport)).use { zipInputStream ->
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

                    require( totalBytes <= m_ReportLimits.MaxZippedBytes )
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
}

fun Route.ReceiveReport( )
{
    post("/upload")
    {
        val conf = call.application.environment.config;

        val limits = ReportLimits(
            MaxZippedBytes = conf.propertyOrNull("monoworks.reports.maxZippedBytes")?.getString()?.toLong() ?: (100L * 1024L * 1024L),
            MaxTotalBytes = conf.propertyOrNull("monoworks.reports.maxTotalBytes")?.getString()?.toLong() ?: (250L * 1024L * 1024L),
            MaxZipEntries = conf.propertyOrNull("monoworks.reports.maxZipEntries")?.getString()?.toInt() ?: 100
        );

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
            return@post;
        }

        val crashReport = CrashReport( report, limits );
        crashReport.Store( Path(conf.propertyOrNull("monoworks.server.reportDir")?.getString() ?: "reports/") );



    }

}