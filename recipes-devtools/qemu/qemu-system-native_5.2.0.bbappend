FILESEXTRAPATHS_prepend := "${THISDIR}/patches:"

SRC_URI_append = " file://0001-add-virt-foo-in-meson-build.patch \
                  file://0001-device-name.patch \
                  file://0002-memory-bus.patch \
                  file://0003-generating-interrupts.patch \
                  file://0004-virt_foo-device.patch \
                  file://0005-flattened-device-tree.patch \
                 "
# TARGET_CFLAGS += "-g"
