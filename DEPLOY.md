# Deployment Instructions

## One-off activities for Windows

1. Ensure that Python is installed and accessible via command line. You may have to 
   ```
   setx PATH "%PATH%;C:\Users\[Username]\AppData\Local\Microsoft\WindowsApps\[SomethingWithPython]\"
   ```
3. Use something like `python3 -m pip install ufbt` to install the [micro Flipper Build Tool](https://github.com/flipperdevices/flipperzero-ufbt)

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
