##Images_Stitching_NodeJS_Server 

###Commands needed to install Node.js as well as all the modules needed to run the app. 

- sudo apt-get install nodejs
- sudo apt-get install npm
- sudo ln -s /usr/bin/nodejs /usr/bin/node

Now, if we have installed it correctly, after checking versions we should see:

- $ node -v
v4.2.1  
- $ npm -v
2.14.7

In order to install the app, everything is declared inside package.json so that only executing <i>npm install</i> in the main directory should be enough. 