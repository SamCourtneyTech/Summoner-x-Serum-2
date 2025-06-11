# CI/CD Setup Guide

This repository includes automated CI/CD pipelines for both the C++ audio plugin and AWS Lambda backend.

## 🏗️ What Gets Built/Deployed

### Audio Plugin (C++)
- **Triggers**: Changes to `Summoner-x-Serum-2/` directory
- **Platforms**: macOS (AU/VST3) and Windows (VST3)
- **Output**: GitHub releases with downloadable plugin bundles

### AWS Lambda Backend
- **Triggers**: Changes to `Summoner AWS Backend/` directory
- **Target**: Automatic deployment to AWS Lambda function
- **Features**: Dependency management, size optimization, rollback support

## 🔧 Required Setup

### 1. GitHub Repository Secrets

Add these secrets in your GitHub repository settings (`Settings > Secrets and variables > Actions`):

#### AWS Credentials
```
AWS_ACCESS_KEY_ID=your_aws_access_key
AWS_SECRET_ACCESS_KEY=your_aws_secret_key
```

#### S3 Bucket (for large Lambda deployments)
```
S3_DEPLOYMENT_BUCKET=your-lambda-deployment-bucket
```

### 2. AWS Lambda Function Setup

Ensure your Lambda function exists and is configured:
- **Function name**: `summoner-aws-backend` (or update in workflow)
- **Runtime**: Python 3.9
- **Region**: us-east-2 (or update in workflow)

### 3. IAM Permissions

Your AWS credentials need these permissions:

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "lambda:UpdateFunctionCode",
                "lambda:UpdateFunctionConfiguration",
                "lambda:InvokeFunction"
            ],
            "Resource": "arn:aws:lambda:us-east-2:*:function:summoner-aws-backend"
        },
        {
            "Effect": "Allow",
            "Action": [
                "s3:PutObject",
                "s3:GetObject",
                "s3:DeleteObject",
                "s3:ListBucket"
            ],
            "Resource": [
                "arn:aws:s3:::your-lambda-deployment-bucket",
                "arn:aws:s3:::your-lambda-deployment-bucket/*"
            ]
        }
    ]
}
```

## 🚀 How It Works

### Automatic Triggers

1. **Push to `main`**: Deploys both plugin and lambda (if changed)
2. **Push to `develop`**: Builds plugin only (no release)
3. **Pull Requests**: Builds plugin for testing
4. **Manual Trigger**: Can force build/deploy via GitHub Actions UI

### Smart Change Detection

The pipeline automatically detects what changed:
- Plugin changes → Build C++ plugin
- Backend changes → Deploy Lambda
- Both changed → Do both
- No changes → Skip everything

### Build Matrix

**Plugin builds across multiple platforms:**
- macOS: Xcode build → AU/VST3 components
- Windows: Visual Studio 2022 → VST3 plugins

**Lambda deployment features:**
- Dependency optimization
- Size checking (auto-S3 for large packages)
- Configuration updates
- Deployment testing

## 📁 Workflow Files

- **`.github/workflows/build-plugin.yml`**: C++ plugin builds
- **`.github/workflows/deploy-lambda.yml`**: AWS Lambda deployment
- **`.github/workflows/ci-cd.yml`**: Main orchestrator workflow

## 🔍 Monitoring

### GitHub Actions
- View builds at: `https://github.com/your-username/your-repo/actions`
- Each workflow provides detailed logs and artifacts

### AWS Lambda
- Monitor deployments in AWS Lambda console
- Check CloudWatch logs for runtime issues

## 🛠️ Customization

### Update Build Settings

Edit the workflow files to customize:

```yaml
# In build-plugin.yml
env:
  BUILD_TYPE: Release  # or Debug
  PLUGIN_NAME: "Your Plugin Name"

# In deploy-lambda.yml
env:
  AWS_REGION: us-east-2  # Your AWS region
  LAMBDA_FUNCTION_NAME: your-function-name
```

### Add Notification Services

Uncomment and configure notification services in `ci-cd.yml`:
- Slack webhooks
- Discord notifications
- Email alerts

### Platform Support

To add Linux builds, extend the matrix in `build-plugin.yml`:

```yaml
- name: Linux
  os: ubuntu-latest
  build_path: LinuxMakefile
  cmake_generator: "Unix Makefiles"
```

## 🔧 Troubleshooting

### Plugin Build Issues
- Check Xcode/Visual Studio versions in workflow
- Verify JUCE project configuration
- Review build logs in Actions tab

### Lambda Deployment Issues
- Verify AWS credentials and permissions
- Check function name and region settings
- Monitor package size (50MB limit)

### Common Problems

1. **Large Lambda Package**: Automatically handled via S3 upload
2. **Build Failures**: Check platform-specific build logs
3. **Permission Errors**: Verify IAM permissions
4. **Missing Secrets**: Add required repository secrets

## 📊 Status Badges

Add these to your README to show build status:

```markdown
![Build Status](https://github.com/your-username/your-repo/workflows/Complete%20CI/CD%20Pipeline/badge.svg)
![Plugin Build](https://github.com/your-username/your-repo/workflows/Build%20Audio%20Plugin/badge.svg)
![Lambda Deploy](https://github.com/your-username/your-repo/workflows/Deploy%20AWS%20Lambda/badge.svg)
```

## 🎯 Manual Triggers

Use the GitHub Actions UI to manually trigger builds with options:
- Force Lambda deployment (even without backend changes)
- Force plugin build and release
- Useful for testing or emergency deployments