# USBMode=default        USB-OTG (TinyUSB), required for HID
# PartitionScheme=custom uses the partitions.csv in the sketch folder
SKETCH="fleabyte"
FQBN="esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=16M,PSRAM=disabled,PartitionScheme=custom"

# Set at compile time: the core opens USB before setup() runs.
# No spaces, arduino-cli splits build properties on them.
USB_PRODUCT_NAME="Fleabyte"
USB_MANUFACTURER_NAME="LilyGO"

BUILD_PROPS=(
  --build-property
  "compiler.cpp.extra_flags=-DUSB_PRODUCT=\"${USB_PRODUCT_NAME}\" -DUSB_MANUFACTURER=\"${USB_MANUFACTURER_NAME}\""
)
