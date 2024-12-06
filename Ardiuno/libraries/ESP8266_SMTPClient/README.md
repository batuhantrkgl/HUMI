# SMTPClient Library

The `SMTPClient` library allows you to send emails using an ESP8266 microcontroller. It supports sending plain text emails as well as emails with attachments.

## Features

- Connect to an SMTP server
- Authenticate using username and password
- Send plain text emails
- Send emails with attachments

## Installation

1. Download the library and place it in the `libraries` folder of your Arduino IDE.
2. Include the library in your sketch:

```cpp
#include <SMTPClient.h>
```

## Usage
Initialize the Client

```cpp
SMTPClient smtp;
smtp.begin("smtp.example.com", 587);
smtp.setCredentials("your_username", "your_password");
```

Send a Plain Text Email

```cpp
bool success = smtp.sendMail("sender@example.com", "recipient@example.com", "Subject", "Email body");
if (success) {
    Serial.println("Email sent successfully!");
} else {
    Serial.println("Failed to send email.");
}
```

Send an Email with Attachment

```cpp
bool success = smtp.sendMailWithAttachment("sender@example.com", "recipient@example.com", "Subject", "Email body", "/path/to/attachment.txt");
if (success) {
    Serial.println("Email with attachment sent successfully!");
} else {
    Serial.println("Failed to send email with attachment.");
}
```

## API Reference
> `bool begin(const String& server, uint16_t port)`

Initialize the SMTP client with the server address and port.

> `void setCredentials(const String& username, const String& password)`

Set the username and password for SMTP authentication.

> `bool sendMail(const String& sender, const String& recipient, const String& subject, const String& message)`

Send a plain text email.

> `bool sendMailWithAttachment(const String& sender, const String& recipient, const String& subject, const String& message, const String& filePath)`

## License
This project is licensed under the [MIT License](licence)