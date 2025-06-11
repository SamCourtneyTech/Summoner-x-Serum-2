import os
from fastapi import FastAPI, HTTPException, Depends
from fastapi.security import OAuth2AuthorizationCodeBearer
from fastapi import Request, HTTPException
from mangum import Mangum
import boto3
from jose import jwt, JWTError
from pydantic import BaseModel
from typing import Dict
import stripe
import openai
from dotenv import load_dotenv
import requests
import json
import sys
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Load environment variables (for local testing)
load_dotenv()

# FastAPI app
app = FastAPI()

# AWS clients
try:
    cognito_client = boto3.client("cognito-idp", region_name="us-east-2")
    logger.info("Cognito client initialized successfully")
    print("Cognito client initialized successfully")
    sys.stdout.flush()
except Exception as e:
    logger.error("Failed to initialize Cognito client: %s", str(e))
    print("Failed to initialize Cognito client:", str(e))
    sys.stdout.flush()

dynamodb = boto3.resource("dynamodb", region_name="us-east-2")
secrets_client = boto3.client("secretsmanager")
table = dynamodb.Table("SummonerUsers")

# Cognito settings
COGNITO_USER_POOL_ID = "us-east-2_pvCcMvaRP"
COGNITO_APP_CLIENT_ID = "77ottlt6s5ntp1jup4av1r62m3"
COGNITO_REGION = "us-east-2"
JWKS_URL = f"https://cognito-idp.{COGNITO_REGION}.amazonaws.com/{COGNITO_USER_POOL_ID}/.well-known/jwks.json"

# Stripe settings
stripe.api_key = os.getenv("STRIPE_SECRET_KEY")

# Get ChatGPT API key from Secrets Manager
def get_chatgpt_key():
    secret = secrets_client.get_secret_value(SecretId="SummonerChatGPTKey3")
    return secret["SecretString"]

# Pydantic models
class UserInput(BaseModel):
    input: str

class PurchaseRequest(BaseModel):
    amount: int  # Amount in cents (e.g., 500 = $5)
    credits: int  # Credits to add

class LoginRequest(BaseModel):
    email: str | None = None
    password: str | None = None
    code: str | None = None
    redirect_uri: str | None = None

# JWT validation
async def get_current_user(token: str = Depends(OAuth2AuthorizationCodeBearer(
    authorizationUrl="auth", tokenUrl="token"
))):
    logger.info(f"Validating token: {token[:20]}...")
    print(f"Validating token: {token[:20]}...")
    sys.stdout.flush()
    try:
        jwks = requests.get(JWKS_URL).json()
        logger.info("JWKS fetched: %s", jwks)
        print("JWKS fetched:", jwks)
        sys.stdout.flush()
        payload = jwt.decode(
            token,
            jwks,
            algorithms=["RS256"],
            audience=COGNITO_APP_CLIENT_ID,
            issuer=f"https://cognito-idp.{COGNITO_REGION}.amazonaws.com/{COGNITO_USER_POOL_ID}"
        )
        logger.info("Token payload: %s", payload)
        print("Token payload:", payload)
        sys.stdout.flush()
        return payload["sub"]
    except JWTError as e:
        logger.error("JWT validation error: %s", str(e))
        print("JWT validation error:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=401, detail="Invalid token")
    except Exception as e:
        logger.error("Unexpected error in get_current_user: %s", str(e))
        print("Unexpected error in get_current_user:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=500, detail="Token validation failed")

class ConfirmRequest(BaseModel):
    email: str
    confirmation_code: str

@app.post("/confirm-signup")
async def confirm_signup(request: ConfirmRequest):
    try:
        cognito_client.confirm_sign_up(
            ClientId=COGNITO_APP_CLIENT_ID,
            Username=request.email,
            ConfirmationCode=request.confirmation_code
        )
        logger.info("User confirmed: %s", request.email)
        print("User confirmed:", request.email)
        sys.stdout.flush()
        return {"status": "success", "message": "User confirmed"}
    except cognito_client.exceptions.ClientError as e:
        logger.error("Confirm signup error: %s", str(e))
        print("Confirm signup error:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        logger.error("Confirmation failed: %s", str(e))
        print("Confirmation failed:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=500, detail=f"Confirmation failed: {str(e)}")

# Endpoints
@app.post("/signup")
async def signup(request: LoginRequest):
    try:
        # Sign up user in Cognito
        response = cognito_client.sign_up(
            ClientId=COGNITO_APP_CLIENT_ID,
            Username=request.email,
            Password=request.password,
            UserAttributes=[{"Name": "email", "Value": request.email}]
        )
        logger.info("Sign up response: %s", response)
        print("Sign up response:", response)
        sys.stdout.flush()
        # Get the user ID (sub) from Cognito
        user = cognito_client.admin_get_user(
            UserPoolId=COGNITO_USER_POOL_ID,
            Username=request.email
        )
        user_sub = user["Username"]
        logger.info("User sub: %s", user_sub)
        print("User sub:", user_sub)
        sys.stdout.flush()
        # Create a record in DynamoDB
        table.put_item(
            Item={
                "userId": user_sub,
                "email": request.email,
                "credits": 0
            }
        )
        logger.info("User added to DynamoDB: %s", request.email)
        print("User added to DynamoDB:", request.email)
        sys.stdout.flush()
        return {"status": "success", "message": "User signed up and added to database"}
    except cognito_client.exceptions.ClientError as e:
        logger.error("Signup error: %s", str(e))
        print("Signup error:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        logger.error("Signup failed: %s", str(e))
        print("Signup failed:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=500, detail=f"Signup failed: {str(e)}")

@app.post("/login")
async def login(request: LoginRequest):
    logger.info("Received login request: %s", request.dict())
    print("Received login request:", request.dict())
    sys.stdout.flush()
    if request.code and request.redirect_uri:
        try:
            logger.info("Exchanging authorization code: %s with redirect_uri: %s", request.code, request.redirect_uri)
            print("Exchanging authorization code:", request.code, "with redirect_uri:", request.redirect_uri)
            sys.stdout.flush()
            # Use the Cognito /token endpoint to exchange the code
            token_url = f"https://us-east-2pvccmvarp.auth.us-east-2.amazoncognito.com/oauth2/token"
            headers = {"Content-Type": "application/x-www-form-urlencoded"}
            # Cognito expects form-encoded data, not JSON
            data = {
                "grant_type": "authorization_code",
                "client_id": COGNITO_APP_CLIENT_ID,
                "code": request.code,
                "redirect_uri": request.redirect_uri
            }
            logger.info("Sending token request to Cognito with data: %s", data)
            print("Sending token request to Cognito with data:", data)
            sys.stdout.flush()
            response = requests.post(token_url, headers=headers, data=data)
            response_json = response.json()
            logger.info("Token endpoint response status: %d, body: %s", response.status_code, response_json)
            print("Token endpoint response status:", response.status_code, "body:", response_json)
            sys.stdout.flush()
            if response.status_code != 200:
                logger.error("Token endpoint failed with status %d: %s", response.status_code, response_json)
                print("Token endpoint failed with status", response.status_code, ":", response_json)
                sys.stdout.flush()
                raise HTTPException(status_code=400, detail=response_json.get("error_description", "Failed to exchange code"))
            access_token = response_json.get("access_token")
            id_token = response_json.get("id_token")
            if not access_token or not id_token:
                logger.error("AccessToken or IdToken missing in token response: %s", response_json)
                print("AccessToken or IdToken missing in token response:", response_json)
                sys.stdout.flush()
                raise HTTPException(status_code=400, detail="AccessToken or IdToken missing")
            return {
                "access_token": access_token,
                "id_token": id_token
            }
        except Exception as e:
            logger.error("Error exchanging code: %s", str(e))
            print("Error exchanging code:", str(e))
            sys.stdout.flush()
            raise HTTPException(status_code=500, detail=f"Unexpected error: {str(e)}")
    elif request.email and request.password:
        try:
            logger.info("Authenticating user with email: %s", request.email)
            print("Authenticating user with email:", request.email)
            sys.stdout.flush()
            response = cognito_client.initiate_auth(
                AuthFlow="USER_PASSWORD_AUTH",
                AuthParameters={"USERNAME": request.email, "PASSWORD": request.password},
                ClientId=COGNITO_APP_CLIENT_ID
            )
            logger.info("Cognito response: %s", response)
            print("Cognito response:", response)
            sys.stdout.flush()
            return {
                "access_token": response["AuthenticationResult"]["AccessToken"],
                "id_token": response["AuthenticationResult"]["IdToken"]
            }
        except cognito_client.exceptions.NotAuthorizedException:
            logger.error("Invalid credentials for email: %s", request.email)
            print("Invalid credentials for email:", request.email)
            sys.stdout.flush()
            raise HTTPException(status_code=401, detail="Invalid credentials")
        except Exception as e:
            logger.error("Unexpected error in password auth: %s", str(e))
            print("Unexpected error in password auth:", str(e))
            sys.stdout.flush()
            raise HTTPException(status_code=500, detail=f"Unexpected error: {str(e)}")
    logger.error("Invalid login request: %s", request.dict())
    print("Invalid login request:", request.dict())
    sys.stdout.flush()
    raise HTTPException(status_code=400, detail="Invalid login request")

@app.post("/purchase-credits")
async def purchase_credits(request: PurchaseRequest, user_id: str = Depends(get_current_user)):
    try:
        # Create a Stripe Checkout Session
        session = stripe.checkout.Session.create(
            payment_method_types=["card"],
            line_items=[
                {
                    "price_data": {
                        "currency": "usd",
                        "product": "prod_S870Z9V37Rndid",  # Replace with your actual Product ID
                        "unit_amount": request.amount,  # Amount in cents (e.g., 1000 for $10.00)
                    },
                    "quantity": 1,
                }
            ],
            mode="payment",
            success_url="https://yourdomain.com/success?session_id={CHECKOUT_SESSION_ID}",  # Replace with your success URL
            cancel_url="https://yourdomain.com/cancel",  # Replace with your cancel URL
            metadata={"user_id": user_id, "credits": str(request.credits)},  # Store user_id and credits for later
        )
        logger.info("Stripe session created for user_id: %s", user_id)
        print("Stripe session created for user_id:", user_id)
        sys.stdout.flush()
        return {"status": "success", "checkout_url": session.url}
    except stripe.error.StripeError as e:
        logger.error("Stripe error: %s", str(e))
        print("Stripe error:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/generate-parameters")
async def generate_parameters(user_input: UserInput, user_id: str = Depends(get_current_user)):
    logger.info("Reached /generate-parameters endpoint for user_id: %s", user_id)
    print("Reached /generate-parameters endpoint for user_id:", user_id)
    sys.stdout.flush()
    response = table.get_item(Key={"userId": user_id})
    logger.info("DynamoDB response: %s", response)
    print("DynamoDB response:", response)
    sys.stdout.flush()
    user = response.get("Item")
    if not user:
        logger.error("User not found in DynamoDB: %s", user_id)
        print("User not found in DynamoDB:", user_id)
        sys.stdout.flush()
        raise HTTPException(status_code=404, detail="User not found")
    credits = int(user.get("credits", 0))
    logger.info("User credits: %d", credits)
    print("User credits:", credits)
    sys.stdout.flush()
    if credits < 1:
        logger.error("Insufficient credits detected for user_id: %s", user_id)
        print("Insufficient credits detected for user_id:", user_id)
        sys.stdout.flush()
        raise HTTPException(status_code=402, detail="Insufficient credits")
    table.update_item(
        Key={"userId": user_id},
        UpdateExpression="SET credits = credits - :val",
        ExpressionAttributeValues={":val": 1}
    )
    logger.info("Credits updated for user_id: %s", user_id)
    print("Credits updated for user_id:", user_id)
    sys.stdout.flush()
    logger.info("Calling ChatGPT with input: %s", user_input.input)
    print("Calling ChatGPT with input:", user_input.input)
    sys.stdout.flush()
    openai.api_key = get_chatgpt_key()
    prompt = f"Interpret the user's request with creativity within the specified ranges and default values, leveraging sound design knowledge to produce engaging and innovative soundscapes using the provided Serum VST parameters. While every response should include all 123 parameters in order formatted as {{\"Parameter Name\", \"Value\"}} in a consistent list, allow for variations that reflect musicality and style. The user's request will be inputted at the bottom of this prompt. Follow these guidelines: Use the full spectrum of provided values and descriptions to address specific or abstract prompts (e.g., \"bright and plucky\", \"deep and textured\") while staying within bounds. Be imaginative in assigning values to create sound textures that meet the user's description, but adhere strictly to parameter names and ensure all 123 parameters are included every time. Return the parameters in the format {{\"Parameter Name\", \"Value\"}}, even if a parameter's default value remains unchanged. Here are the 123 parameters and their default values: [{{\"Parameter Name\": \"Env1 Atk\", \"Value\": \"0.5 ms\"}}, {{\"Parameter Name\": \"Env1 Hold\", \"Value\": \"0.0 ms\"}}, {{\"Parameter Name\": \"Env1 Dec\", \"Value\": \"1.00 s\"}}, {{\"Parameter Name\": \"Env1 Sus\", \"Value\": \"0.0 dB\"}}, {{\"Parameter Name\": \"Env1 Rel\", \"Value\": \"15 ms\"}}, {{\"Parameter Name\": \"Osc A On\", \"Value\": \"on\"}}, {{\"Parameter Name\": \"A UniDet\", \"Value\": \"0.25\"}}, {{\"Parameter Name\": \"A UniBlend\", \"Value\": \"75\"}}, {{\"Parameter Name\": \"A WTPos\", \"Value\": \"Sine\"}}, {{\"Parameter Name\": \"A Pan\", \"Value\": \"0\"}}, {{\"Parameter Name\": \"A Vol\", \"Value\": \"75%\"}}, {{\"Parameter Name\": \"A Unison\", \"Value\": \"1\"}}, {{\"Parameter Name\": \"A Octave\", \"Value\": \"0 Oct\"}}, {{\"Parameter Name\": \"A Semi\", \"Value\": \"0 semitones\"}}, {{\"Parameter Name\": \"A Fine\", \"Value\": \"0 cents\"}}, {{\"Parameter Name\": \"Fil Type\", \"Value\": \"MG Low 12\"}}, {{\"Parameter Name\": \"Fil Cutoff\", \"Value\": \"425 Hz\"}}, {{\"Parameter Name\": \"Fil Reso\", \"Value\": \"10%\"}}, {{\"Parameter Name\": \"Filter On\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Fil Driv\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"Fil Var\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"Fil Mix\", \"Value\": \"100%\"}}, {{\"Parameter Name\": \"OscA>Fil\", \"Value\": \"on\"}}, {{\"Parameter Name\": \"OscB>Fil\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"OscN>Fil\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"OscS>Fil\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Osc N On\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Noise Pitch\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"Noise Level\", \"Value\": \"25%\"}}, {{\"Parameter Name\": \"Osc S On\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Sub Osc Level\", \"Value\": \"75%\"}}, {{\"Parameter Name\": \"SubOscOctave\", \"Value\": \"0 Oct\"}}, {{\"Parameter Name\": \"SubOscShape\", \"Value\": \"Sine\"}}, {{\"Parameter Name\": \"Osc B On\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"B UniDet\", \"Value\": \"0.25\"}}, {{\"Parameter Name\": \"B UniBlend\", \"Value\": \"75\"}}, {{\"Parameter Name\": \"B WTPos\", \"Value\": \"1\"}}, {{\"Parameter Name\": \"B Pan\", \"Value\": \"0\"}}, {{\"Parameter Name\": \"B Vol\", \"Value\": \"75%\"}}, {{\"Parameter Name\": \"B Unison\", \"Value\": \"1\"}}, {{\"Parameter Name\": \"B Octave\", \"Value\": \"0 Oct\"}}, {{\"Parameter Name\": \"B Semi\", \"Value\": \"0 semitones\"}}, {{\"Parameter Name\": \"B Fine\", \"Value\": \"0 cents\"}}, {{\"Parameter Name\": \"Hyp Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Hyp_Rate\", \"Value\": \"40%\"}}, {{\"Parameter Name\": \"Hyp_Detune\", \"Value\": \"25%\"}}, {{\"Parameter Name\": \"Hyp_Retrig\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Hyp_Wet\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"Hyp_Unision\", \"Value\": \"4\"}}, {{\"Parameter Name\": \"HypDim_Size\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"HypDim_Mix\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"Dist Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Dist_Mode\", \"Value\": \"Tube\"}}, {{\"Parameter Name\": \"Dist_PrePost\", \"Value\": \"Off\"}}, {{\"Parameter Name\": \"Dist_Freq\", \"Value\": \"330 Hz\"}}, {{\"Parameter Name\": \"Dist_BW\", \"Value\": \"1.9\"}}, {{\"Parameter Name\": \"Dist_L/B/H\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"Dist_Drv\", \"Value\": \"25%\"}}, {{\"Parameter Name\": \"Dist_Wet\", \"Value\": \"100%\"}}, {{\"Parameter Name\": \"Flg Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Flg_Rate\", \"Value\": \"0.08 Hz\"}}, {{\"Parameter Name\": \"Flg_BPM_Sync\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Flg_Dep\", \"Value\": \"100%\"}}, {{\"Parameter Name\": \"Flg_Feed\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"Flg_Stereo\", \"Value\": \"180deg.\"}}, {{\"Parameter Name\": \"Flg_Wet\", \"Value\": \"100%\"}}, {{\"Parameter Name\": \"Phs Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Phs_Rate\", \"Value\": \"0.08 Hz\"}}, {{\"Parameter Name\": \"Phs_BPM_Sync\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Phs_Dpth\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"Phs_Frq\", \"Value\": \"600 Hz\"}}, {{\"Parameter Name\": \"Phs_Feed\", \"Value\": \"80%\"}}, {{\"Parameter Name\": \"Phs_Stereo\", \"Value\": \"180deg.\"}}, {{\"Parameter Name\": \"Phs_Wet\", \"Value\": \"100%\"}}, {{\"Parameter Name\": \"Cho Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Cho_Rate\", \"Value\": \"0.08 Hz\"}}, {{\"Parameter Name\": \"Cho_BPM_Sync\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Cho_Dly\", \"Value\": \"5.0 ms\"}}, {{\"Parameter Name\": \"Cho_Dly2\", \"Value\": \"0.0 ms\"}}, {{\"Parameter Name\": \"Cho_Dep\", \"Value\": \"26.0 ms\"}}, {{\"Parameter Name\": \"Cho_Feed\", \"Value\": \"10%\"}}, {{\"Parameter Name\": \"Cho_Filt\", \"Value\": \"1000 Hz\"}}, {{\"Parameter Name\": \"Cho_Wet\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"Dly Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Dly_Feed\", \"Value\": \"40%\"}}, {{\"Parameter Name\": \"Dly_BPM_Sync\", \"Value\": \"on\"}}, {{\"Parameter Name\": \"Dly_Link\", \"Value\": \"Unlink, Link\"}}, {{\"Parameter Name\": \"Dly_TimL\", \"Value\": \"1/4\"}}, {{\"Parameter Name\": \"Dly_TimR\", \"Value\": \"1/4\"}}, {{\"Parameter Name\": \"Dly_BW\", \"Value\": \"6.8\"}}, {{\"Parameter Name\": \"Dly_Freq\", \"Value\": \"849 Hz\"}}, {{\"Parameter Name\": \"Dly_Mode\", \"Value\": \"Normal\"}}, {{\"Parameter Name\": \"Dly_Wet\", \"Value\": \"30%\"}}, {{\"Parameter Name\": \"Comp Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"Cmp_Thr\", \"Value\": \"-18.1 dB\"}}, {{\"Parameter Name\": \"Cmp_Att\", \"Value\": \"90.1 ms\"}}, {{\"Parameter Name\": \"Cmp_Rel\", \"Value\": \"90 ms\"}}, {{\"Parameter Name\": \"CmpGain\", \"Value\": \"0.0 dB\"}}, {{\"Parameter Name\": \"CmpMBnd\", \"Value\": \"Normal\"}}, {{\"Parameter Name\": \"Comp_Wet\", \"Value\": \"100\"}}, {{\"Parameter Name\": \"Rev Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"VerbSize\", \"Value\": \"35%\"}}, {{\"Parameter Name\": \"Decay\", \"Value\": \"4.7 s\"}}, {{\"Parameter Name\": \"VerbLoCt\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"VerbHiCt\", \"Value\": \"35%\"}}, {{\"Parameter Name\": \"Spin Rate\", \"Value\": \"25%\"}}, {{\"Parameter Name\": \"Verb Wet\", \"Value\": \"20%\"}}, {{\"Parameter Name\": \"EQ Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"EQ FrqL\", \"Value\": \"210 Hz\"}}, {{\"Parameter Name\": \"EQ Q L\", \"Value\": \"60%\"}}, {{\"Parameter Name\": \"EQ VolL\", \"Value\": \"0.0 dB\"}}, {{\"Parameter Name\": \"EQ TypL\", \"Value\": \"Shelf\"}}, {{\"Parameter Name\": \"EQ TypeH\", \"Value\": \"Shelf\"}}, {{\"Parameter Name\": \"EQ FrqH\", \"Value\": \"2041 Hz\"}}, {{\"Parameter Name\": \"EQ Q H\", \"Value\": \"60%\"}}, {{\"Parameter Name\": \"EQ VolH\", \"Value\": \"0.0\"}}, {{\"Parameter Name\": \"FX Fil Enable\", \"Value\": \"off\"}}, {{\"Parameter Name\": \"FX Fil Type\", \"Value\": \"MG Low 6\"}}, {{\"Parameter Name\": \"FX Fil Freq\", \"Value\": \"330 Hz\"}}, {{\"Parameter Name\": \"FX Fil Reso\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"FX Fil Drive\", \"Value\": \"0%\"}}, {{\"Parameter Name\": \"FX Fil Pan\", \"Value\": \"50%\"}}, {{\"Parameter Name\": \"FX Fil Wet\", \"Value\": \"100%\"}}] Here are those 123 parameter's respective ranges that you can choose from: [{{\"Parameter Name\": \"Env1 Atk\", \"Value\": \"0.0 ms - 32.0 s\"}}, {{\"Parameter Name\": \"Env1 Hold\", \"Value\": \"0.0 ms - 32.0 s\"}}, {{\"Parameter Name\": \"Env1 Dec\", \"Value\": \"0.0 ms - 32.0 s\"}}, {{\"Parameter Name\": \"Env1 Sus\", \"Value\": \"-inf dB - 0.0 dB\"}}, {{\"Parameter Name\": \"Env1 Rel\", \"Value\": \"0.0ms - 32.0s\"}}, {{\"Parameter Name\": \"Osc A On\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"A UniDet\", \"Value\": \"0.00 - 1.00\"}}, {{\"Parameter Name\": \"A UniBlend\", \"Value\": \"0 - 100\"}}, {{\"Parameter Name\": \"A WTPos\", \"Value\": \"Sine, Saw, Triangle, Square, Pulse, Half Pulse, Inv-Phase Saw\"}}, {{\"Parameter Name\": \"A Pan\", \"Value\": \"-50 - 50\"}}, {{\"Parameter Name\": \"A Vol\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"A Unison\", \"Value\": \"1 - 16\"}}, {{\"Parameter Name\": \"A Octave\", \"Value\": \"-4 Oct, -3 Oct, -2 Oct, -1 Oct, 0 Oct, 1 Oct, 2 Oct, 3 Oct, 4 Oct\"}}, {{\"Parameter Name\": \"A Semi\", \"Value\": \"-12 semitones - +12 semitones\"}}, {{\"Parameter Name\": \"A Fine\", \"Value\": \"-100 cents - 100 cents\"}}, {{\"Parameter Name\": \"Fil Type\", \"Value\": \"MG Low 6, MG Low 12, MG Low 18, MG Low 24, Low 6, Low 12, Low 18, Low 24, High 6, High 12, High 18, High 24, Band 12, Band 24, Peak 12, Peak 24, Notch 12, Notch 24, LH 6, LH 12, LB 12, LP 12, LN 12, HB 12, HP 12, HN 12, BP 12, PP 12, PN 12, NN 12, L/B/H 12, L/B/H 24, L/P/H 12, L/P/H 24, L/N/H 12, L/N/H 24, B/P/N 12, B/P/N 24, Cmb +, Cmb -, Cmb L6+, Cmb L6-, Cmb H6+, Cmb H6-, Cmb HL6+, Cmb HL6-, Flg +, Flg -, Flg L6+, Flg L6-, Flg H6+, Flg H6-, Flg HL6+, Flg HL6-, Phs 12+, Phs 12-, Phs 24+, Phs 24-, Phs 36+, Phs 36-, Phs 48+, Phs 48-, Phs 48L6+, Phs 48L6-, Phs 48H6+, Phs 48H6-, Phs 48HL6+, Phs 48HL6-, FPhs 12HL6+, FPhs 12HL6-, Low EQ 6, Low EQ 12, Band EQ 12, High EQ 6, High EQ 12, Ring Mod, Ring Modx2, SampHold, SampHold-, Combs, Allpasses, Reverb, French LP, German LP, Add Bass, Formant-I, Formant-II, Formant-III, Bandreject, Dist.Comb 1 LP, Dist.Comb 1 BP, Dist.Comb 2 LP, Dist.Comb 2 BP, Scream LP, Scream BP\"}}, {{\"Parameter Name\": \"Fil Cutoff\", \"Value\": \"8 Hz - 22050 Hz\"}}, {{\"Parameter Name\": \"Fil Reso\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Filter On\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Fil Driv\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Fil Var\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Fil Mix\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"OscA>Fil\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"OscB>Fil\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"OscN>Fil\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"OscS>Fil\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Osc N On\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Noise Pitch\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Noise Level\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Osc S On\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Sub Osc Level\", \"Value\": \"0%-100%\"}}, {{\"Parameter Name\": \"SubOscOctave\", \"Value\": \"-4 Oct, -3 Oct, -2 Oct, -1 Oct, 0 Oct, 1 Oct, 2 Oct, 3 Oct, 4 Oct\"}}, {{\"Parameter Name\": \"SubOscShape\", \"Value\": \"Sine, RoundRect, Triangle, Saw, Square, Pulse\"}}, {{\"Parameter Name\": \"Osc B On\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"B UniDet\", \"Value\": \"0.00 - 1.00\"}}, {{\"Parameter Name\": \"B UniBlend\", \"Value\": \"0 - 100\"}}, {{\"Parameter Name\": \"B WTPos\", \"Value\": \"Sine, Saw, Triangle, Square, Pulse, Half Pulse, Inv-Phase Saw\"}}, {{\"Parameter Name\": \"B Pan\", \"Value\": \"-50 - 50\"}}, {{\"Parameter Name\": \"B Vol\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"B Unison\", \"Value\": \"1 - 16\"}}, {{\"Parameter Name\": \"B Octave\", \"Value\": \"-4 Oct, -3 Oct, -2 Oct, -1 Oct, 0 Oct, 1 Oct, 2 Oct, 3 Oct, 4 Oct\"}}, {{\"Parameter Name\": \"B Semi\", \"Value\": \"-12 semitones - +12 semitones\"}}, {{\"Parameter Name\": \"B Fine\", \"Value\": \"-100 cents - 100 cents\"}}, {{\"Parameter Name\": \"Hyp Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Hyp_Rate\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Hyp_Detune\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Hyp_Retrig\", \"Value\": \"off - Retrig\"}}, {{\"Parameter Name\": \"Hyp_Wet\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Hyp_Unision\", \"Value\": \"0 - 7\"}}, {{\"Parameter Name\": \"HypDim_Size\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"HypDim_Mix\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Dist Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Dist_Mode\", \"Value\": \"Tube, SoftClip, HardClip, Diode 1, Diode 2, Lin.Fold, Sin Fold, Zero-Square, Downsample, Asym, Rectify, X-Shaper, X-Shaper (Asym), Sine Shaper, Stomp Box, Tape Stop\"}}, {{\"Parameter Name\": \"Dist_PrePost\", \"Value\": \"Off, Pre, Post\"}}, {{\"Parameter Name\": \"Dist_Freq\", \"Value\": \"8 Hz, 13290 Hz\"}}, {{\"Parameter Name\": \"Dist_BW\", \"Value\": \"0.1 - 7.6\"}}, {{\"Parameter Name\": \"Dist_L/B/H\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Dist_Drv\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Dist_Wet\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Flg Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Flg_Rate\", \"Value\": \"0.00 Hz - 20.00 Hz\"}}, {{\"Parameter Name\": \"Flg_BPM_Sync\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Flg_Dep\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Flg_Feed\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Flg_Stereo\", \"Value\": \"22 Hz - 200\"}}, {{\"Parameter Name\": \"Flg_Wet\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Phs Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Phs_Rate\", \"Value\": \"0.00 Hz - 20.00 Hz\"}}, {{\"Parameter Name\": \"Phs_BPM_Sync\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Phs_Dpth\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Phs_Frq\", \"Value\": \"20 Hz - 18000 Hz\"}}, {{\"Parameter Name\": \"Phs_Feed\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Phs_Stereo\", \"Value\": \"0 deg. - 360 deg.\"}}, {{\"Parameter Name\": \"Phs_Wet\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Cho Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Cho_Rate\", \"Value\": \"0.00 Hz - 20.00 Hz\"}}, {{\"Parameter Name\": \"Cho_BPM_Sync\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Cho_Dly\", \"Value\": \"0.0 ms - 20.0 ms\"}}, {{\"Parameter Name\": \"Cho_Feed\", \"Value\": \"0% - 95%\"}}, {{\"Parameter Name\": \"Cho_Filt\", \"Value\": \"50 Hz - 20000 Hz\"}}, {{\"Parameter Name\": \"Cho_Wet\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Dly Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Dly_BPM_Sync\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Dly_TimL\", \"Value\": \"1.00 - 501.00\"}}, {{\"Parameter Name\": \"Dly_TimR\", \"Value\": \"1.00 - 501.00\"}}, {{\"Parameter Name\": \"Dly_Freq\", \"Value\": \"40 Hz - 18000 Hz\"}}, {{\"Parameter Name\": \"Dly_Mode\", \"Value\": \"Normal, Ping-Pong, Tap->Delay\"}}, {{\"Parameter Name\": \"Dly_Wet\", \"Value\": \"0% - 100%\"}}, {{\"Parameter Name\": \"Comp Enable\", \"Value\": \"off, on\"}}, {{\"Parameter Name\": \"Cmp_Thr\", \"Value\": \"0.0 dB - 120.0 dB\"}}, {{\"Parameter Name\": \"Cmp_Att\", \"Value\": \"0.1 ms - 1000.0 ms\"}}, {{\"Parameter Name\": \"Cmp_Rel\", \"Value\": \"0.1 ms - 999.1 ms\"}}, {{\"Parameter Name\": \"CmpGain\", \"Value\": \"0.0 dB - 30.1 dB\"}}, {{\"Parameter Name\": \"CmpMBnd\", \"Value\": \"Normal, MultBand\"}}, {{\"Parameter Name\": \"Comp_Wet\", \"Value\": \"0 - 100\"}}] To facilitate accurate parsing and handling of your requests, please provide parameter adjustments in JSON format when possible. This ensures the correct interpretation and application of your specifications for this C++ program. Don't add newline characters. Even if the request doesn't seem to be related to the sound designing task, respond with the list: DO NOT RESPOND WITH ANYTHING BUT THE LIST! User input: {user_input.input}. Return as JSON."
    try:
        response = openai.ChatCompletion.create(
            model="gpt-3.5-turbo",  # Revert to gpt-3.5-turbo for compatibility
            messages=[{"role": "user", "content": prompt}],
            response_format={"type": "json_object"}
        )
        parameters = response.choices[0].message.content
        if isinstance(parameters, str):
            parameters = json.loads(parameters)
        logger.info("ChatGPT response: %s", parameters)
        print("ChatGPT response:", parameters)
        sys.stdout.flush()
        return parameters
    except Exception as e:
        logger.error("OpenAI API error: %s", str(e))
        print("OpenAI API error:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=500, detail=f"OpenAI API error: {str(e)}")
@app.post("/get-credits")
async def get_credits(user_id: str = Depends(get_current_user)):
    try:
        response = table.get_item(Key={"userId": user_id})
        user = response.get("Item")
        if not user:
            logger.error("User not found for user_id: %s", user_id)
            print("User not found for user_id:", user_id)
            sys.stdout.flush()
            raise HTTPException(status_code=404, detail="User not found")
        credits = user.get("credits", 0)
        logger.info("Credits fetched for user_id %s: %d", user_id, credits)
        print("Credits fetched for user_id", user_id, ":", credits)
        sys.stdout.flush()
        return {"credits": credits}
    except Exception as e:
        logger.error("Failed to fetch credits: %s", str(e))
        print("Failed to fetch credits:", str(e))
        sys.stdout.flush()
        raise HTTPException(status_code=500, detail=f"Failed to fetch credits: {str(e)}")

@app.post("/refresh")
async def refresh_token(request: Request):
    try:
        body = await request.json()
        refresh_token = body.get("refresh_token")
        if not refresh_token:
            raise HTTPException(status_code=400, detail="Missing refresh token")

        response = cognito_client.initiate_auth(
            AuthFlow="REFRESH_TOKEN_AUTH",
            AuthParameters={"REFRESH_TOKEN": refresh_token},
            ClientId=COGNITO_APP_CLIENT_ID
        )

        return {
            "access_token": response["AuthenticationResult"]["AccessToken"],
            "id_token": response["AuthenticationResult"]["IdToken"]
        }
    except cognito_client.exceptions.NotAuthorizedException:
        raise HTTPException(status_code=401, detail="Invalid or expired refresh token")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Refresh failed: {str(e)}")

def webhook_handler(event, context):
    logger.info("Webhook handler received event: %s", event)
    print("Webhook handler received event:", event)
    sys.stdout.flush()
    raw_body = event.get("body")
    if not isinstance(raw_body, str):
        response = {"statusCode": 400, "body": json.dumps({"detail": "Body must be a string"})}
        logger.info("Webhook response: %s", response)
        print("Webhook response:", response)
        sys.stdout.flush()
        return response

    headers = event.get("headers", {})
    signature = headers.get("Stripe-Signature") or headers.get("stripe-signature")
    if not signature:
        response = {"statusCode": 400, "body": json.dumps({"detail": "Missing Stripe-Signature header"})}
        logger.info("Webhook response: %s", response)
        print("Webhook response:", response)
        sys.stdout.flush()
        return response

    logger.info("Received signature: %s", signature)
    print("Received signature:", signature)
    logger.info("Raw body for verification: %s", raw_body)
    print("Raw body for verification:", raw_body)
    sys.stdout.flush()

    webhook_secret = "whsec_8bed60b149ee8710ccea0dacd40faf1d12c38d2bacab75b89d50a612eb7f49f9"
    stripe_event = None
    try:
        # Ensure raw_body is treated as bytes for verification
        stripe_event = stripe.Webhook.construct_event(
            payload=raw_body.encode("utf-8"),
            sig_header=signature,
            secret=webhook_secret
        )
    except ValueError as e:
        response = {"statusCode": 400, "body": json.dumps({"detail": "Invalid payload"})}
        logger.info("Webhook response: %s", response)
        print("Webhook response:", response)
        sys.stdout.flush()
        return response
    except stripe.error.SignatureVerificationError as e:
        response = {"statusCode": 400, "body": json.dumps({"detail": "Invalid signature"})}
        logger.info("Webhook response: %s", response)
        print("Webhook response:", response)
        sys.stdout.flush()
        return response

    # Parse the payload for processing after verification
    try:
        payload = json.loads(raw_body)
    except json.JSONDecodeError:
        response = {"statusCode": 400, "body": json.dumps({"detail": "Invalid JSON payload"})}
        logger.info("Webhook response: %s", response)
        print("Webhook response:", response)
        sys.stdout.flush()
        return response

    if stripe_event["type"] == "checkout.session.completed":
        session = stripe_event["data"]["object"]
        user_id = session["metadata"].get("user_id")
        credits_to_add = int(session["metadata"].get("credits", 0))
        if not user_id or not credits_to_add:
            response = {"statusCode": 400, "body": json.dumps({"detail": "Missing metadata"})}
            logger.info("Webhook response: %s", response)
            print("Webhook response:", response)
            sys.stdout.flush()
            return response
        try:
            table.update_item(
                Key={"userId": user_id},
                UpdateExpression="SET credits = credits + :val",
                ExpressionAttributeValues={":val": credits_to_add},
                ConditionExpression="attribute_exists(userId)"
            )
            response = {"statusCode": 200, "body": json.dumps({"status": "success", "message": "Credits updated"})}
            logger.info("Webhook response: %s", response)
            print("Webhook response:", response)
            sys.stdout.flush()
            return response
        except Exception as e:
            response = {"statusCode": 500, "body": json.dumps({"detail": f"Failed to update credits: {str(e)}"})}
            logger.info("Webhook response: %s", response)
            print("Webhook response:", response)
            sys.stdout.flush()
            return response

    response = {"statusCode": 200, "body": json.dumps({"status": "success", "message": "Event received"})}
    logger.info("Webhook response: %s", response)
    print("Webhook response:", response)
    sys.stdout.flush()
    return response

#Handler

def handler(event, context):
    logger.info("Received event: %s", json.dumps(event, indent=2))
    print("Received event:", json.dumps(event, indent=2))
    sys.stdout.flush()
    logger.info("Extracted path: %s", (
        event.get("path") or
        event.get("requestContext", {}).get("path") or
        event.get("requestContext", {}).get("http", {}).get("path") or
        event.get("resource", "")
    ))
    print("Extracted path:", (
        event.get("path") or
        event.get("requestContext", {}).get("path") or
        event.get("requestContext", {}).get("http", {}).get("path") or
        event.get("resource", "")
    ))
    sys.stdout.flush()
    path = (
        event.get("path") or
        event.get("requestContext", {}).get("path") or
        event.get("requestContext", {}).get("http", {}).get("path") or
        event.get("resource", "")
    )
    if path == "/webhook" or "webhook" in path.lower():
        return webhook_handler(event, context)
    return Mangum(app)(event, context)

# Test comment to trigger CI/CD deployment - fixed region and function name
