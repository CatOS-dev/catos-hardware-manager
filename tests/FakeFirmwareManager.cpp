#include <QCoreApplication>
#include <QFile>
#include <cstdio>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto arguments = app.arguments();
    QFile record(qEnvironmentVariable("CATOS_FIRMWARE_TEST_RECORD"));
    if (!record.open(QIODevice::WriteOnly | QIODevice::Append)) return 1;
    record.write(arguments.mid(1).join(QLatin1Char(' ')).toUtf8() + '\n');
    if (arguments.contains(QStringLiteral("refresh"))) return 0;
    if (arguments.contains(QStringLiteral("get-devices"))) {
        std::puts(R"({"Devices":[{"DeviceId":"test-id","Name":"Test device","Version":"1.0"}]})");
        return 0;
    }
    if (arguments.contains(QStringLiteral("get-updates"))) {
        if (qEnvironmentVariable("CATOS_FIRMWARE_TEST_MODE") == QLatin1String("empty")) return 2;
        if (qEnvironmentVariable("CATOS_FIRMWARE_TEST_MODE") == QLatin1String("invalid")) { std::puts("not-json"); return 0; }
        std::puts(R"({"Devices":[{"DeviceId":"test-id","Releases":[{"Version":"2.0","Summary":"Fix device reliability"}]}]})");
        return 0;
    }
    return 1;
}
