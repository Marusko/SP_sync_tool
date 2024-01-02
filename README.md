# SynK

A tool for synchronizing files from the client to the server. 

It only works on **Linux**.

## Launching
> The app does not save the port number or IP address of server

### First launch
```
Enter 
[ 's' / 'server' ] to run as server
[ 'c' / 'client' ] to run as client:
```

### Server
```
[Q] Enter server port number: 
```
Here enter your desired port number and hit `Enter`, the server will then turn ON

### Client
```
[Q] Enter server IP address: 
```
Here enter the IP address of your server and hit `Enter`
```
[Q] Enter server port number: 
```
Here enter the server port and hit `Enter`, the client will then turn ON and connect to the server at the entered IP address and port

## Controls

### Client controls
Available commands:
  * `slist` - get list of files on server
  * `sync` - synchronize files with server
  * `autosync [on|off]` - turn on / off automatic synchronization
  * `autocheck [s]` - in seconds, sets how often the sync folder should be scanned for file updates
  * `setpath [path]` - sets the path to the folder whose contents will be synchronized
  * `delete [file]` - deletes the specified file
  * `sdelete [file]` - deletes the specified file on server
  * `restore [file]` - restores the specified file
  * `exit` - closes the application

### Server controls
  * `exit` - turn OFF the server, for this command to work, at least one client **must be connected**

## Restrictions
  * Editing files must be done when the `client` is on because it will not be synchronized
  * Creating new files is possible even if `client` is not turned on
  * Files **must** not have a space in the name
  * Deleting files is done **exclusively via the client**
  * Files will remain on the server until you issue a command to delete them on the server
  * **Two** clients need to be launched for synchronization between two local folders

---

&copy; [Matúš Suský](https://github.com/Marusko), [David Kučera](https://github.com/david-kucera) 2024
