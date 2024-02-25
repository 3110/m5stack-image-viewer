# Image Viewer for M5Stack Series

This program displays images stored on the LittleFS file system of the M5Stack series devices one after another.

There are two display modes available:

* Manual Mode  
  Press the A button to switch and display images one by one.
* Auto Mode  
  Displays images at intervals (in milliseconds) specified in the configuration file. It can also display images at random intervals up to the specified display interval.

## How to Compile

Compile in the [PlatformIO IDE](https://platformio.org/platformio-ide) environment. Please select the environment appropriate for your model.

| Model            | Environment              |
| :--------------- | :------------------------|
| M5Stack BASIC    | env:m5stack-basic        |
| M5Stack Fire     | env:m5stack-fire         |
| M5Go             | env:m5stack-m5go         |
| M5Stack Core2    | env:m5stack-core2        |
| M5Stack Core3    | env:m5stack-core3        |
| M5Stick C        | env:m5stick-c            |
| M5Stick C Plus   | env:m5stick-c-plus       |
| M5Stick C Plus2  | env:m5stick-c-plus2      |
| M5ATOM S3        | env:m5stack-atoms3       |
| M5Dial           | env:m5stack-dial         |
| M5Cardputer      | env:m5stack-cardputer    |
| CoreInk          | env:m5stack-coreink      |
| M5Paper          | env:m5stack-paper        |

## Configuration File

The following settings can be configured in the `data/image-viewer.json` configuration file:

* `AutoMode`  
  Turn auto display mode on (`true`) or off (`false`).
* `AutoModeInterval`  
  Interval for switching images in auto display mode (milliseconds).
* `AutoModeRandomized`  
  Turn random interval switching mode on (`true`) or off (`false`).

If the configuration file is missing, auto mode is off (`false`), the switch interval is 3 seconds (3000 milliseconds), and random interval switching mode is off (`false`).

```json
{
  "AutoMode": false,
  "AutoModeInterval": 3000,
  "AutoModeRandomized": false
}
```

Turning on the random interval switching mode will switch images at random intervals between 0 milliseconds and the interval specified by `AutoModeInterval`.

This configuration file will be uploaded to the device's file system along with the images during the "Uploading Images to Display" process in the next step.

## Uploading Images to Display

Place image files (PNG, JPEG, BMP) in the `data` directory and upload them to the device using one of the following methods:

* Select "Upload Filesystem Image" from the PlatformIO menu.  
* Execute `pio run --target uploadfs` from the command line.

All files placed in the `data` directory, including the `data/image-viewer.json` configuration file, will be uploaded to the device.

## How to Run

When the program is launched, it sequentially displays image files (PNG, JPEG, BMP) found in the file system. Normally, it starts in the mode specified in the configuration file. If the A button is pressed while starting, it switches to auto mode regardless of the settings.

If an IMU is available on your device, the display orientation automatically changes to match the screen's orientation.

Upon startup, the following is displayed. If there is no configuration file, the `Config:` information will not be shown.

```text
Image Viewer v1.0.0
Config:
 /image-viewer.json
 AutoMode: false
 Interval: 3000ms
 Randomized: false
Mode:
 Manual, Auto or Auto(Forced)
Image Files:
 Image File 1
 Image File 2
 ...
 Image File N
```

If no image files are found on the file system, the following message is displayed:

```text
Image Viewer v1.0.0
Config:
 /image-viewer.json
 AutoMode: false
 Interval: 3000ms
 Randomized: false
Mode:
 Manual, Auto or Auto(Forced)
No image files found
```

After displaying the list of images for a certain period (default is 3 seconds), the screen displays images according to the selected mode.

## How to Create Custom Firmware

To generate a custom firmware file for distribution, you can use either of the following methods:

* Select "Custom/Generate User Custom" from the PlatformIO menu.
* Execute `pio run --target firmware` from the command line.

A file named `[model_name]_image_viewer_firmware_[version].bin` will be generated in the `firmware` directory.

For settings related to the firmware being generated, please refer to the following items in the `image-viewer` section of the `platformio.ini` file:

* Model name  
  `custom_firmware_target`  
  A string representing each model is defined for each environment.  
  Example: M5Stack Basic = m5basic
* Firmware name  
  `custom_firmware_name` (Default: `image_viewer`)
* File name containing the firmware version  
  `custom_firmware_version_file` (Default: `ImageViewer.cpp`)  
  Extracts the value of `X.Y.Z` from the code written as `"vX.Y.Z"`.
* Firmware extension (dot not required)  
  `custom_firmware_suffix` (Default: `bin`)
* Destination directory  
  `custom_firmware_dir` (Default: `firmware`)
