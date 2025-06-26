# Deployment Instructions

## One-off activities for Windows

1. Ensure that Python is installed and accessible via command line. You may have to 
   ```
   setx PATH "%PATH%;C:\Users\[Username]\AppData\Local\Microsoft\WindowsApps\[SomethingWithPython]\"
   ```
3. Use something like `python3 -m pip install ufbt` to install the [micro Flipper Build Tool](https://github.com/flipperdevices/flipperzero-ufbt)
4. Add the uFBT folder to your Windows PATH environment variable.
5. Start ufBT for the first time with `python -m ufbt` to ensure that it downloads and installs the necessary SDK components from the official firmware's release update channel. This includes a toolchain built for your platform. The output may look as follows:
```
16:23:00.842 [I] Deploying SDK for f7
16:23:00.842 [I] Fetching version info for UpdateChannel.RELEASE from https://update.flipperzero.one/firmware/directory.json
16:23:01.478 [I] Using version: 1.3.4
16:23:01.478 [I] uFBT SDK dir: C:\Users\flori\.ufbt\current
16:23:08.427 [I] Deploying SDK
16:23:10.316 [I] SDK deployed.
Downloading Windows toolchain..
```

## Things to check before deploying

1.Does the Application Manifest *application.fam* exist and link the correct function?
  ```
 App(
    appid="my_test_app",
    name="My Test App",
    apptype=FlipperAppType.EXTERNAL,
    entry_point="my_test_app",
    cdefines=["APP_MY_TEST"],
    requires=["gui"],
    stack_size=2 * 1024,
    order=20,
)
```
Make sure the *entry_point* matches the function name in your `my_main_file.c` file.


## Building and deploying the artifact to the device

1. Connect your Flipper Zero via USB cable.
