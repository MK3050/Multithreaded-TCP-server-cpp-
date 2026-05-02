#pragma once

bool send_all(int sock, const char* data, int length);

void setNonBlocking(int fd);