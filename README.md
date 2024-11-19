# ServerClient

## Build
Build projects `Client`, `Server`, `WebcamIntegrated`

## Run
Before running these execute files, remember to put Server.exe and WebCamIntegrated.exe into the same directory
### On the server machine (Run as Administrator to works with WindowsServices)
```
Server.exe
```
### On the client machine
```
Client.exe
```
Enter IP: ```<Enter the server's IP Address>```
Client: ```<TASKNAME> [file_name]```

examples: ```STARTPROCESS notepad.exe```

list of commands:

```SHUTDOWN```

```STARTPROCESS taskName.exe```

```KILLPROCESS taskName.exe```

```LISTPROCESS file.txt```

```STARTSERVICE svcName```

```STOPSERVICE svcName```

```LISTSERVICES file.txt```

```SCREENCAPTURE file.png```

```SENDFILE fileName.abc```

```TURNONCAMERA```

```TURNOFFCAMERA```

```END```

