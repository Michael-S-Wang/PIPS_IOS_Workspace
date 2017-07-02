# PIPS_IOS_Workspace
PIPS IOS Development Workspace


##Development

This project contains an XCode workspace to facilitate building both the algo library and the IOS Interface, as it currently stands this repo holds the algorithm code while the IOS interface is pulled in via a 'git submodule'.  When the static library is built it places the necessary current headers and lib file into the 

### Initial setup of the folder
1. clone git@github.com:Michael-S-Wang/PIPS_IOS_Workspace.git
2. cd into PIPS_IOS_Workspace
3 run './setup.sh' to properlly init

### Commiting Changes to the Static Library
This is standard git.

#### Publish The Library to the Interface
Note that when you push the changes to the library it doesn't mean that the Interface project (which can be pulled seperatly for development by those without privlege to the algos) you'll need to complete the next section to push out the static library and headers to the interface.

### Commiting Changes to the Interface
Since the interface is in a submodule, you'll need to CD into that directory first and then issue a standard 'git add', 'git commit', and 'git push' commands to publish  the latest changes.
 - Additionally note that this doesn't update the reference point in the workspace project, you'll need to CD back to the root project and commit the updated refrence for PIPS_IOS and push it to result in futures pulls of the worksace having the correct starting point to the project.


### Getting the latest Interface Changes
Since the interface is in a submodule, you'll need to CD into that directory first and then issue a standard 'git pull' command to retrieve the latest files. 

