# BV-BRC application execution shepherd

## Overview

This module implements an application execution shepherd, a program that is invoked when the BV-BRC cluster scheduler runs an application backend on behalf of a user.

It provides the following services:

- Conveys application standard output and standard error streams back to the [BV-BRC application service](https://github.com/BV-BRC/app_service) for real-time examination by adminstrators and users.
- Logs subprocess startup and shutdown to enable detailed logging of application execution
- Monitors memory and CPU usage of the application and conveys that to the application service
