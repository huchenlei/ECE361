# Text Conferencing Pro
## Version
1.0
## Client actions
### Login
`/login <username> <password> <server ip> <server port>`  
Login on server  
Available usernames and cooresponding password can be found on `users.c`
### Logout
`/logout`  
Leave all joined sessions and quit
### Session
`/createsession <session id>` create new session  
`/joinsession <session id>` join a session  
`/leavesession <session id>` leave a joined session  
`/switchsession <session_id>` switch to another joined session; All messages later would be forward to that channel.  
`/invite <username> <session id>` invite another user to join the session specified  
### Others
`/quit` logout the user and quit the conferencing app  
`/list` list all sessions on server  
`<text>` send a message to current session the user is in  

## Testing
- run `make test` to recompile the program and start the server
- run `./client_test.tv <port number>` for some basic functionality test for single user interactions


