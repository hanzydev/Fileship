---
title: IIS
description: Serve Fileship on Windows Server or Windows 11 through IIS with URL Rewrite and Application Request Routing, with WebSockets and large uploads.
---

# IIS

On Windows you can put Fileship behind IIS with the **URL Rewrite** and **Application Request Routing (ARR)** modules. [Caddy for Windows](caddy.md#windows) is simpler if you don't already run IIS. Read the [overview](overview.md) first if you have not.

> [!NOTE]
> This configuration follows Microsoft's documentation for ARR and has not been tested yet.

## 1. Install IIS and the modules

As administrator in PowerShell. On Windows 11:

```powershell
Enable-WindowsOptionalFeature -Online -FeatureName IIS-WebServerRole, IIS-WebServer, IIS-WebSockets, IIS-RequestFiltering -All
```

On Windows Server:

```powershell
Install-WindowsFeature Web-Server, Web-WebSockets, Web-Filtering -IncludeManagementTools
```

Then install [URL Rewrite](https://www.iis.net/downloads/microsoft/url-rewrite) and [Application Request Routing 3.0](https://www.iis.net/downloads/microsoft/application-request-routing) from iis.net.

The **WebSocket Protocol** feature is required, or socket.io falls back to long polling.

## 2. Turn on the proxy and keep the Host header

```powershell
& "$env:windir\system32\inetsrv\appcmd.exe" set config -section:system.webServer/proxy /enabled:"True" /preserveHostHeader:"True" /reverseRewriteHostInResponseHeaders:"False" /timeout:"00:05:00" /commit:apphost
```

`preserveHostHeader` keeps your domain in the links Fileship returns. ARR adds `X-Forwarded-For` with the client address by default.

## 3. Allow the forwarded headers as server variables

URL Rewrite can only set request headers that are allowed as server variables:

```powershell
$appcmd = "$env:windir\system32\inetsrv\appcmd.exe"
& $appcmd set config -section:system.webServer/rewrite/allowedServerVariables /+"[name='HTTP_X_FORWARDED_PROTO']" /commit:apphost
& $appcmd set config -section:system.webServer/rewrite/allowedServerVariables /+"[name='HTTP_X_FILESHIP_CLIENT_IP']" /commit:apphost
```

## 4. Create the site

Create a site for your domain (for example in `C:\inetpub\fileship`) with an HTTPS binding and your certificate (see [SSL certificates](ssl.md#iis)), then put this `web.config` in its folder:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<configuration>
    <system.webServer>
        <rewrite>
            <rules>
                <rule name="Fileship" stopProcessing="true">
                    <match url="(.*)" />
                    <serverVariables>
                        <set name="HTTP_X_FORWARDED_PROTO" value="https" />
                        <set name="HTTP_X_FILESHIP_CLIENT_IP" value="{REMOTE_ADDR}" />
                    </serverVariables>
                    <action type="Rewrite" url="http://127.0.0.1:3000/{R:1}" />
                </rule>
            </rules>
        </rewrite>
        <security>
            <requestFiltering>
                <requestLimits maxAllowedContentLength="4294967295" />
            </requestFiltering>
        </security>
        <webSocket enabled="true" />
    </system.webServer>
</configuration>
```

- `maxAllowedContentLength` raises IIS's body limit (about 28.6 MB by default, close to the 25 MB chunk size) to its maximum of 4 GB, so ShareX and scripts can upload large files.
- `X-Fileship-Client-IP` is set to the real client address, so visitors cannot send their own.
- For an HTTP-only site, set `HTTP_X_FORWARDED_PROTO` to `http`.

`127.0.0.1:3000` is Docker Desktop's forwarded port with [Docker](../install/windows.md#with-docker), or the app [without Docker](../install/windows.md#without-docker).

## Check it

Follow [After the proxy works](overview.md#after-the-proxy-works).
