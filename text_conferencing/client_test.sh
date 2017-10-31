#!/usr/bin/expect
set timeout 50
puts "Running Testcase for Text Conferencing client"
# Start the client
spawn ./client

expect  {
    "Welcome to Text Conferencing Pro v1\.0" {
        send "/login Chenlei chenlei localhost 5000\n"
    }
}

expect {
    ".*Success.*": {
        send "/quit\n"
    }
}

interact
