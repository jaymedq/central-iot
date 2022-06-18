# How to setup a web development enviroment

## Step 1: Install python requirements 
```cmd
pip install -r requirements.txt
```

## Step 2: If there is no Profile, create one with the following settings:
```procfile
web: gunicorn app: app
```