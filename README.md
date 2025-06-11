# Plugin Backend 
 
This is the AWS backend for the JUCE plugin, handling authentication via AWS Cognito and other API endpoints. 
 
## Prerequisites 
 
- Python 3.9+ installed 
- AWS CLI configured with your credentials 
- Git installed 
 
## Setup and Deployment 
 
### 1. Clone the Repository 
 
Clone the repo to your local machine: 
 
``` 
git clone https://github.com/<your-username>/plugin-backend.git 
cd plugin-backend 
``` 
 
### 2. Set Up a Virtual Environment 
 
Create and activate a virtual environment to manage dependencies: 
 
``` 
python3 -m venv venv 
source venv/bin/activate  # On Windows, use `venv\Scripts\activate` 
``` 
 
### 3. Install Dependencies 
 
Install the required Python packages listed in `requirements.txt`, download and user docker. By itself pip seems to not get the dependencies right. Run: 
 
``` 
docker run -v "C:\Users\User\Documents\plugin-backend:/app" -w /app python:3.12-slim pip install -r requirements.txt -t .
``` 
 
### 4. Install Dependencies in the Project Directory 
 
AWS Lambda needs dependencies in the project folder. Install them directly into the directory: 
 
``` 
pip install -r requirements.txt -t . 
``` 
 
### 5. Zip the Project for Lambda 
 
Create a zip file for deployment, excluding the `.git` folder: 
 
``` 
zip -r plugin-backend.zip . -x ".git/*" 
``` 
 
### 6. Upload to AWS Lambda 
 
Upload the zip file to your Lambda function via the AWS Management Console: 
 
1. Go to the AWS Lambda Console. 
2. Select your function (e.g., `plugin-backend-function`). 
3. In the "Code" tab, choose "Upload from" > ".zip file". 
4. Upload `plugin-backend.zip`. 
5. Click "Deploy". 
 
Alternatively, use the AWS CLI: 
 
``` 
aws lambda update-function-code --function-name plugin-backend-function --zip-file fileb://plugin-backend.zip 
``` 
 
### 7. Test the Function 
 
Test your Lambda function using the AWS Console or a tool like Postman: 
 
- If testing the `/login` endpoint (Cognito authentication), use the Invoke URL from API Gateway. 
- Example payload: `{"email": "test@example.com", "password": "test123"}` 
 
## Notes 
 
- Ensure your Cognito User Pool has test users set up (email verified) for login testing. 
- The Lambda function expects `main.py` with a handler (e.g., `lambda_handler`). 
- Keep `.env` files out of the repo for security (listed in `.gitignore`). 
