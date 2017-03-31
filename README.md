# 0x0ACE
Solutions for the 0x0ACE ARG (http://80.233.134.208/)


### What is 0x0ACE?
0x0ACE is an ARG (augmented reality game) where the goal is to solve coding challenges.



### Which challenges are currently available?
There are 3 challgenges available right now:
* [#1](http://80.233.134.208/)
* [#2](http://5.9.247.121/d34dc0d3)
* [#3](http://80.233.134.207/0x00000ACE.html)

To access #2 and #3 you need to have generated a key by completing #1.
#1 highlights a base64 string which is easy to decode with any online tool. #2 and #3, however, are refreshed every 5 seconds and must therefore be solved programmatically.



### What are the steps to solving #2 a.k.a d34dc0d3?
To solve #2 you first need to send an HTTP GET request and parse the HTML response to get the values of the left and right limits of a set. You then need to HTTP POST the odd prime numbers between them, and a verification string.



### What are the steps to solving #3 a.k.a 0x00000ACE?
To solve #3 you need to write your own interpreter for some bytecode with specifications on the page, and then HTTP POST the final register values back to the server.



### When is the next challenge?
The next challenge will be released on the 30th of April.
