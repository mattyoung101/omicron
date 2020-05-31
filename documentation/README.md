# About

This is the documentation for Team Omicron 2020. We use MkDocs to convert markdown files into a full website to document 
our development of the robot.

# Instructions for team members who are the only people who are going to read this hopefully

To start, please clone the teamomicron git repo in the root directory of this git project with the following command:

`git clone https://github.com/TeamOmicron/teamomicron.github.io.git`

You will need to install mkdocs and the material theme in order to deploy the site. To do this, you can use pip:

`pip install mkdocs`

`pip install mkdocs-material`

`pip install markdown`

To deploy the website, go to the path of the github team (which should be ~/teamomicron.github.io) and use this command:

`mkdocs gh-deploy --config-file ../documentation/mkdocs.yml --remote-branch master`

Note: if you haven't added mkdocs to path, you'll need to prefix the command with `python -m`

Be sure to push all your changes to Github from the root directory.

**If you want to run a local copy** of the server, type `mkdocs serve` in your terminal.
