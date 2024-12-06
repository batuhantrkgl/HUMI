#include "SMTPClient.h"
#include "Base64.h" // Include the Base64 library

SMTPClient::SMTPClient() {}

bool SMTPClient::begin(const String& server, uint16_t port) {
    this->server = server;
    this->port = port;
    return true;
}

void SMTPClient::setCredentials(const String& username, const String& password) {
    this->username = username;
    this->password = password;
}

bool SMTPClient::sendCommand(const String& command, const String& expectedResponse) {
    client.println(command);
    String response = readResponse();
    return response.startsWith(expectedResponse);
}

String SMTPClient::readResponse() {
    String response = "";
    while (client.available()) {
        response += client.readString();
    }
    return response;
}

String SMTPClient::encodeFileToBase64(const String& filePath) {
    File file = SD.open(filePath, FILE_READ);
    if (!file) {
        return "";
    }

    String encoded = "";
    while (file.available()) {
        uint8_t buffer[3];
        size_t bytesRead = file.read(buffer, 3);
        encoded += Base64::encode(buffer, bytesRead);
    }

    file.close();
    return encoded;
}

bool SMTPClient::sendMail(const String& sender, const String& recipient, const String& subject, const String& message) {
    if (!client.connect(server.c_str(), port)) return false;

    if (!sendCommand("EHLO Arduino", "250")) return false;

    String auth = "AUTH LOGIN";
    if (!sendCommand(auth, "334")) return false;

    client.println(Base64::encode((const uint8_t*)username.c_str(), username.length()));
    if (!readResponse().startsWith("334")) return false;

    client.println(Base64::encode((const uint8_t*)password.c_str(), password.length()));
    if (!readResponse().startsWith("235")) return false;

    if (!sendCommand("MAIL FROM:<" + sender + ">", "250")) return false;
    if (!sendCommand("RCPT TO:<" + recipient + ">", "250")) return false;
    if (!sendCommand("DATA", "354")) return false;

    String emailContent = "Subject: " + subject + "\r\n\r\n" + message + "\r\n.";
    if (!sendCommand(emailContent, "250")) return false;

    sendCommand("QUIT", "221");
    client.stop();

    return true;
}

bool SMTPClient::sendMailWithAttachment(const String& sender, const String& recipient, const String& subject, const String& message, const String& filePath) {
    if (!client.connect(server.c_str(), port)) return false;

    if (!sendCommand("EHLO Arduino", "250")) return false;

    String auth = "AUTH LOGIN";
    if (!sendCommand(auth, "334")) return false;

    client.println(Base64::encode((const uint8_t*)username.c_str(), username.length()));
    if (!readResponse().startsWith("334")) return false;

    client.println(Base64::encode((const uint8_t*)password.c_str(), password.length()));
    if (!readResponse().startsWith("235")) return false;

    if (!sendCommand("MAIL FROM:<" + sender + ">", "250")) return false;
    if (!sendCommand("RCPT TO:<" + recipient + ">", "250")) return false;
    if (!sendCommand("DATA", "354")) return false;

    String boundary = "----=_NextPart_000_0000_01D3BD6C.0A0A0A0A";
    String emailContent = "Subject: " + subject + "\r\n";
    emailContent += "MIME-Version: 1.0\r\n";
    emailContent += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n\r\n";
    emailContent += "--" + boundary + "\r\n";
    emailContent += "Content-Type: text/plain; charset=\"UTF-8\"\r\n\r\n";
    emailContent += message + "\r\n\r\n";
    emailContent += "--" + boundary + "\r\n";
    emailContent += "Content-Type: application/octet-stream; name=\"" + filePath + "\"\r\n";
    emailContent += "Content-Transfer-Encoding: Base64\r\n";
    emailContent += "Content-Disposition: attachment; filename=\"" + filePath + "\"\r\n\r\n";
    emailContent += encodeFileToBase64(filePath) + "\r\n";
    emailContent += "--" + boundary + "--\r\n.";

    if (!sendCommand(emailContent, "250")) return false;

    sendCommand("QUIT", "221");
    client.stop();

    return true;
}