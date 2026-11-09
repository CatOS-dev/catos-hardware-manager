// Device discovery against a mocked sysfs/udev tree. Run through
// umockdev-wrapper, which redirects /sys and the udev database.
#include "services/DeviceDiscovery.h"

#include <QtTest>
#pragma push_macro("signals")
#undef signals
#include <umockdev.h>
#pragma pop_macro("signals")

namespace {
const char *const kTopology = R"(P: /devices/pci0000:00/0000:00:01.1
E: SUBSYSTEM=pci
E: DRIVER=pcieport
A: class=0x060400
A: vendor=0x1022
A: device=0x14db

P: /devices/pci0000:00/0000:00:01.1/0000:01:00.0
E: SUBSYSTEM=pci
E: DRIVER=nvidia
E: ID_MODEL_FROM_DATABASE=AD107M [GeForce RTX 4060 Max-Q / Mobile]
E: ID_VENDOR_FROM_DATABASE=NVIDIA Corporation
A: class=0x030000
A: vendor=0x10de
A: device=0x28e0
A: boot_vga=0

P: /devices/pci0000:00/0000:00:08.1/0000:06:00.0
E: SUBSYSTEM=pci
E: DRIVER=amdgpu
E: ID_MODEL_FROM_DATABASE=Raphael
A: class=0x030000
A: vendor=0x1002
A: device=0x164e
A: boot_vga=1

P: /devices/pci0000:00/0000:00:1f.3
E: SUBSYSTEM=pci
E: DRIVER=snd_hda_intel
E: ID_MODEL_FROM_DATABASE=Raptor Lake High Definition Audio
A: class=0x040300
A: vendor=0x8086
A: device=0x7a50

P: /devices/pci0000:00/0000:00:1f.3/sound/card0
E: SUBSYSTEM=sound

P: /devices/pci0000:00/0000:00:14.0
E: SUBSYSTEM=pci
E: DRIVER=xhci_hcd
A: class=0x0c0330
A: vendor=0x8086
A: device=0x7a60

P: /devices/pci0000:00/0000:00:14.0/usb3
E: SUBSYSTEM=usb
E: DEVTYPE=usb_device
E: DRIVER=usb
A: bDeviceClass=09
A: idVendor=1d6b
A: idProduct=0002

P: /devices/pci0000:00/0000:00:14.0/usb3/3-1
E: SUBSYSTEM=usb
E: DEVTYPE=usb_device
E: DRIVER=usb
E: ID_MODEL_FROM_DATABASE=AX211 Bluetooth
A: bDeviceClass=e0
A: idVendor=8087
A: idProduct=0033

P: /devices/pci0000:00/0000:00:14.0/usb3/3-1/3-1:1.0
E: SUBSYSTEM=usb
E: DEVTYPE=usb_interface
E: DRIVER=btusb
A: bInterfaceClass=e0

P: /devices/pci0000:00/0000:00:14.0/usb3/3-1/3-1:1.0/bluetooth/hci0
E: SUBSYSTEM=bluetooth

P: /devices/pci0000:00/0000:00:14.0/usb3/3-2
E: SUBSYSTEM=usb
E: DEVTYPE=usb_device
E: DRIVER=usb
E: ID_MODEL_FROM_DATABASE=YubiKey 5
A: bDeviceClass=00
A: idVendor=1050
A: idProduct=0407

P: /devices/pci0000:00/0000:00:14.0/usb3/3-2/3-2:1.0
E: SUBSYSTEM=usb
E: DEVTYPE=usb_interface
A: bInterfaceClass=0b

P: /devices/pci0000:00/0000:00:14.0/usb3/3-3
E: SUBSYSTEM=usb
E: DEVTYPE=usb_device
E: DRIVER=usb
E: ID_MODEL_FROM_DATABASE=Fingerprint Reader
A: bDeviceClass=ef
A: idVendor=27c6
A: idProduct=609c

P: /devices/pci0000:00/0000:00:14.0/usb3/3-3/3-3:1.0
E: SUBSYSTEM=usb
E: DEVTYPE=usb_interface
A: bInterfaceClass=ff

P: /devices/pci0000:00/0000:00:14.0/usb3/3-4
E: SUBSYSTEM=usb
E: DEVTYPE=usb_device
E: DRIVER=usb
E: ID_MODEL_FROM_DATABASE=Integrated Camera
A: bDeviceClass=ef
A: idVendor=04f2
A: idProduct=b7b4

P: /devices/pci0000:00/0000:00:14.0/usb3/3-4/3-4:1.0
E: SUBSYSTEM=usb
E: DEVTYPE=usb_interface
E: DRIVER=uvcvideo
A: bInterfaceClass=0e

P: /devices/pci0000:00/0000:00:14.0/usb3/3-4/3-4:1.0/video4linux/video0
E: SUBSYSTEM=video4linux
E: ID_V4L_CAPABILITIES=:capture:
)";
}

class DeviceDiscoveryTest : public QObject {
    Q_OBJECT
    UMockdevTestbed *testbed_ = nullptr;
    QVector<HardwareDevice> devices_;

    QVector<HardwareDevice> withId(const QString &id) const
    {
        QVector<HardwareDevice> result;
        for (const auto &device : devices_)
            if (device.id == id) result.append(device);
        return result;
    }
    HardwareDevice only(const QString &id) const
    {
        const auto found = withId(id);
        return found.size() == 1 ? found.first() : HardwareDevice{};
    }

private slots:
    void initTestCase()
    {
        if (!qEnvironmentVariable("LD_PRELOAD").contains(QStringLiteral("umockdev")))
            QSKIP("Run through umockdev-wrapper");
        testbed_ = umockdev_testbed_new();
        GError *error = nullptr;
        QVERIFY2(umockdev_testbed_add_from_string(testbed_, kTopology, &error),
                 error ? error->message : "invalid topology");
        devices_ = DeviceDiscovery::enumerate(false);
    }
    void cleanupTestCase()
    {
        if (testbed_) g_object_unref(testbed_);
    }

    void graphicsBehindBridgeKeepsOwnIdentity()
    {
        const auto gpu = only(QStringLiteral("pci:0000:01:00.0"));
        QCOMPARE(gpu.category, DeviceCategory::Graphics);
        QCOMPARE(gpu.vendorId, QStringLiteral("0x10de"));
        QCOMPARE(gpu.productId, QStringLiteral("0x28e0"));
        QCOMPARE(gpu.driver, QStringLiteral("nvidia"));
        QVERIFY(gpu.name.contains(QStringLiteral("RTX 4060")));
    }

    void hybridGraphicsMarksBootDisplay()
    {
        QVERIFY(only(QStringLiteral("pci:0000:06:00.0")).capabilities.contains(QStringLiteral("primary-gpu")));
        QVERIFY(!only(QStringLiteral("pci:0000:01:00.0")).capabilities.contains(QStringLiteral("primary-gpu")));
        QVERIFY(only(QStringLiteral("system:graphics")).capabilities.contains(QStringLiteral("dual-gpu")));
    }

    void soundCardReportsPciDriver()
    {
        const auto audio = only(QStringLiteral("pci:0000:00:1f.3"));
        QCOMPARE(audio.category, DeviceCategory::Audio);
        QCOMPARE(audio.driver, QStringLiteral("snd_hda_intel"));
    }

    void bluetoothAdapterIsListedOnceWithInterfaceDriver()
    {
        const auto entries = withId(QStringLiteral("usb:3-1"));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().category, DeviceCategory::Bluetooth);
        QCOMPARE(entries.first().driver, QStringLiteral("btusb"));
        QCOMPARE(entries.first().vendorId, QStringLiteral("8087"));
    }

    void unboundSmartCardDoesNotBorrowControllerDriver()
    {
        const auto card = only(QStringLiteral("usb:3-2"));
        QCOMPARE(card.category, DeviceCategory::SmartCard);
        QVERIFY2(card.driver.isEmpty(), qPrintable(card.driver));
    }

    void vendorSpecificUsbDeviceStaysDiscoverable()
    {
        const auto reader = only(QStringLiteral("usb:3-3"));
        QVERIFY(reader.capabilities.contains(QStringLiteral("usb-device")));
        QCOMPARE(reader.vendorId, QStringLiteral("27c6"));
        QVERIFY(reader.driver.isEmpty());
    }

    void cameraReportsInterfaceDriverOnce()
    {
        const auto entries = withId(QStringLiteral("usb:3-4"));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().category, DeviceCategory::Camera);
        QCOMPARE(entries.first().driver, QStringLiteral("uvcvideo"));
    }

    void rootHubsAndBridgesAreNotDevices()
    {
        QVERIFY(withId(QStringLiteral("usb:usb3")).isEmpty());
        QVERIFY(withId(QStringLiteral("pci:0000:00:01.1")).isEmpty());
    }
};

QTEST_GUILESS_MAIN(DeviceDiscoveryTest)
#include "DeviceDiscoveryTest.moc"
