# ComputerNetworking
Notice: this project have been tested on
  + IDE: Visual Studio 2022
  + OS: Windows 10

## Preparing

`vcpkg` : You can skip this if you already have vcpkg or 
Visual Studio 2022 version 17.6, which the vcpkg C/C++ package manager is included as an installable component for 
the C++ Desktop and C++ Gaming workloads.

### Installing vcpkg


+ Step 1: Clone vcpkg repository:

	```git clone https://github.com/microsoft/vcpkg.git```

+ Step 2: Run the boostrap: 

	```cd vcpkg; .\bootstrap-vcpkg.bat```

+ Step 3: Integrate vcpkg to MSBuild:
  ```.\vcpkg.exe integrate install```

Step 3: Set the environment variables: 


## Run
Clone this repository and open it

```
git clone https://github.com/iAmHieu2012/ServerClient.git
cd ComputerNetworking
```
Before building files, 

  + You need to change the macro in `Client.cpp`:
      `#define SENDER_MAIL "abc@gmail.com"`
    to the mail which you use to send request to server machine

  + Make sure that you have `vcpkg.json` and `vcpkg_configuration.json`. If not, follow this
    ```
    $env:VCPKG_ROOT = "C:\path\to\vcpkg"
    $env:PATH = "$env:VCPKG_ROOT;$env:PATH"
    vcpkg new --application
    vcpkg add port curl
    vcpkg add port nlohmann-json
    ```   

## Build

Choose `vcpkg` to build projects

`Ctrl` + `Shift` + `B`  to build 3 subprojects `Server`, `Client`, `WebcamIntegrated`

The execute files built are placed in separate folders in `\vcpkg-installed`

`curl` and `nlohmann-json` are auto-generate on your machine too.

## Run
Before running these execute files, remember:
	
+ put `Server.exe` and `WebcamIntegrated.exe` into the same directory on the server machine

+ put `Client.exe`(required) ,`credentials.json`(required, i've put it in the solution directory) and `token.json` (if missing you will be asked to get a new access_token, so you needn't care 'bout it) into the same directory on the client machine

if the `token.json` is missing/error, the client application will ask you to get the authorization code.
You must use this account to get the token: 
the mail of the client machine :
mail account: `hieudapchailo@gmail.com`
password: `Hieu.google.2012`

### On the server machine (Run as Administrator to works with WindowsServices)
```
server.exe
```
### On the client machine
```
client.exe
```

use `SENDER_MAIL` to send email to the client email `hieudapchailo@gmail.com` with:

	To: <__hieudapchailo@gmail.com__>

	Subject: <__Server's IP Address__>

	Body: <__one of the commands below__>

list of commands:

`SHUTDOWN`

`STARTPROCESS taskName.exe`

`KILLPROCESS taskName.exe`

`LISTPROCESS file.txt`

`STARTSERVICE svcName`

`STOPSERVICE svcName`

`LISTSERVICES file.txt`

`SCREENCAPTURE file.png`

`SENDFILE fileName.abc`

`TURNONCAMERA`

`TURNOFFCAMERA`

## How it works

you(SENDER_MAIL: you modify your own email)-->

-- send email of your order -->  Client(Mail: hieudapchailo@gmail.com)

-- send the task -->  Server

-- do the task and send result -->  Client

-- send email of the result -->  you(SENDER_MAIL)



