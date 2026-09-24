# overview for server


**BACKEND**
the backend is a golang webserver in chi, handles storage, communicating with licensing server and more


**FRONTEND**
front end is a react SPA app

on login you land on the dashboard
the app is built around the current campaign id
in the top right is a drop down box with all campaigns -> gotten via GET /rest/campaigns/metadata(just returns UUID + name, name is shown in the drop down)

the landing page is a statisics of the campaign
total users, most common country, users overtime etc

on the left is a panel(can be folded to hide it or show it)
with
- users
- campaign
- builder
- files
- settings

**USERS**
GET /rest/users/{campaign id} -> to list them all
DELETE /rest/users/{campaign id} -> to delete one
list the users of current campaign gotten via

one per line
a user can be doubled clicked which opens a terminal on the bottom half of the page and updates react internal userInUse UUID

**CAMPAIGN**
GET /rest/campaigns/ -> list all campaigns(uuid, name, created at, total users etc)
DELETE /rest/campaigns/{uuid} -> delete campaign
PATCH /rest/campaigns/{uuid} -> edit campaign
POST /rest/campaigns/create -> create campaign
the managemnt for them, can create ones, delete, modify etc 
1 per line



**BUILDER**
POST /rest/builder/create -> create
builder panel
choose type, file etc


**FILES**
GET /rest/files/{campaign id} -> list them all
DELETE /rest/files/{campaign id}/{file id} -> delete one


**SETTINGS**
management area
can update backend ip/domain for licensing server, change password etc
