#!/usr/bin/expect
set timeout 2
set port [lindex $argv 0]

set username "Chenlei"
set userpass "chenlei"
set ip_addr "localhost"
set sessions [list CHENLEI_ROOM ECE_COMMON HAIL_MSE Good_Days RoundRobin]

puts "Running Testcase for Text Conferencing client"
# Start the client
spawn ./client

expect  {
    "Welcome to Text Conferencing Pro v1\.0" {
        send "/login $username $userpass $ip_addr $port\n"
    }
}

expect "Successfully loggged in as Chenlei"

foreach session_name $sessions {
    send "/createsession $session_name\n"
    expect "Successfully created session $session_name"
}

foreach session_name $sessions {
    send "/switchsession $session_name\n"
    # dummy expect
    expect "something"
    send "Hello in $session_name\n"
    expect "\[$session_name\] $username says: Hello in $session_name"
}


foreach session_name $sessions {
    send "/leavesession $session_name\n"
    expect -re "\s+.+\[Server\] leaving session $session_name..."
}

# quit the conference
send "/quit\n"

interact
