This tutorial shows how to setup a Google Apps Script that allows to monitor a gmail inbox and register new users in Allods2 hat based on the incoming emails.
## Use case 1. Register a new login.
To register a new login it needs to send an email from unregistered gmail account to the admin account with subject `create-a2-login`. If you leave body empty, it will generate a random password for you. Otherwise it will use email body to generate a password(not all charactes are allowed for the password).
Protocol Example:
```
FROM: abc@gmail.com
TO: a2admin@gmail.com
Subject: create-a2-login
Body: Hello, can you please craete a login for me?
```
```
FROM: a2admin@gmail.com
TO: abc@gmail.com
Subject: RE: create-a2-login
Body: Your login has been created.
login:"abc" (first 15 letters and digits of your email account),
password: "hellocanyo"
```
## Use case 2. Reset the password
If you forgot your login/password, you can send an email with subject `reset-a2-password`. It will reply you with your login and a new password. If you leave body empty, it will generate a random password for you. Otherwise it will use email body to generate a password(not all charactes are allowed for the password).
Protocol Example:
```
FROM: abc@gmail.com
TO: a2admin@gmail.com
Subject: reset-a2-password
Body: Hello, can you please craete a login for me?
```
```
FROM: a2admin@gmail.com
TO: abc@gmail.com
Subject: RE: create-a2-login
Body: Your password has been reset.
login:"abc",
password: "hellocanyo"
```
# How to setup this email-based admin controller
## Prerequisites
It needs:
- a regular gmail account.
- access to Google Apps Script. All gmail accounts should have this service.

## Setup
### 1. Create Google Apps Script project
Go to https://script.google.com/ and create a new project
![image](https://user-images.githubusercontent.com/12370336/120575034-59228780-c3ee-11eb-8e67-79535614f67c.png)
### 2. Rename the project
You may want to give a meaningful name to the project. But it is not required to do now, so you can skip.
### 3. Copy code
Find the script in this repo https://github.com/serg-bloim/redhat/blob/email-login-controller/email-admin/Code.gs. Copy and paste into the Apps Script editor(delete the standard empty function in the editor). And save the project.
![image](https://user-images.githubusercontent.com/12370336/120575401-06959b00-c3ef-11eb-94ce-adb84ab373a2.png)
### 3.1 Test the code
Now you can test the code works properly.
Send an email to the current gmail account. Set the subject to `create-a2-login`. You can send an email to yourself from the same account)
![image](https://user-images.githubusercontent.com/12370336/120581921-9d675500-c3f9-11eb-8a46-ffab41c772c1.png)
Select the functiona as on the screenshot and hit Run. Once script finishes, check your email.
### 4. Deployment.
A saved version of the script is called a deployment. We need to create a one.
![image](https://user-images.githubusercontent.com/12370336/120576485-d4853880-c3f0-11eb-9c08-acf5d6a129f3.png)
Type smth into Description smth like 'version1' and hit Deploy.
### 5. Setup time trigger
We need to run the code regularly so it will process new emails. For that we need to create a trigger.
![image](https://user-images.githubusercontent.com/12370336/120577933-00092280-c3f3-11eb-946d-72ccb851cde5.png)

Configure like on the screenshot and hit Save.
![image](https://user-images.githubusercontent.com/12370336/120581044-2c736d80-c3f8-11eb-97e7-0a37b248d387.png)

Now it will try to evaluate the permissions for the trigger. The script accesses emails in the gmail inbox and the prompts will tell you about it. Just click Next, Next, Next as on the screenshots. When I tried this on my account it errored out when I did it first time, but for the second time it went well.
![image](https://user-images.githubusercontent.com/12370336/120578033-262ec280-c3f3-11eb-9b7d-3faf795e97f7.png)
![image](https://user-images.githubusercontent.com/12370336/120578161-60985f80-c3f3-11eb-8408-4cc45a1549f7.png)
![image](https://user-images.githubusercontent.com/12370336/120578283-8f163a80-c3f3-11eb-8874-447102203645.png)
![image](https://user-images.githubusercontent.com/12370336/120578349-a7865500-c3f3-11eb-9b83-d27066e8069d.png)
![image](https://user-images.githubusercontent.com/12370336/120578398-bc62e880-c3f3-11eb-8256-d0cc106d9fbd.png)




