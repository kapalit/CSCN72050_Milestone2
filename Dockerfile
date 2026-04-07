# =============================================================
# COIL Robot Command-and-Control GUI  –  Windows Container
# =============================================================
#
# REQUIREMENTS
#   • Docker Desktop in "Windows containers" mode (Settings → Switch to Windows containers)
#   • Host OS and container version must match (ltsc2019 ↔ Windows 10/Server 2019, etc.)
#
# HOW TO BUILD AND RUN
# ------------------------------------------------------------
# 1.  Build the WebServer project first in Visual Studio:
#        Configuration: Release | x64
#        Output:  x64\Release\WebServer.exe
#
# 2.  From the solution root (next to this Dockerfile), run:
#        docker build -t coil-c2gui .
#
# 3.  Start the container (maps host port 8080 to container 8080):
#        docker run -p 8080:8080 coil-c2gui
#
# 4.  Open a browser on the host and go to:
#        http://localhost:8080/
#
# 5.  Start Robot_Simulator.exe on the host:
#        Robot_Simulator.exe <port>
#
#     NOTE: When the container sends UDP to the robot simulator
#     running on the host, use  host.docker.internal  as the IP
#     in the GUI's "Robot IP" field (Docker's built-in alias for
#     the Windows host NIC from inside a Windows container).
# =============================================================

# Windows Server Core LTSC 2019  (≈ 4 GB download, first time only)
FROM mcr.microsoft.com/windows/servercore:ltsc2019

LABEL maintainer="CSCN72050 COIL Group"
LABEL description="Milestone 3 – Robot Command-and-Control Web Server"

WORKDIR C:\\app

# Copy the compiled Release binary and any runtime DLLs it needs.
# If you get "missing DLL" errors, copy the required .dll files alongside
# WebServer.exe before building the image (e.g. VCRUNTIME140.dll from VS).
COPY x64\\Release\\WebServer.exe .

# Expose the CROW HTTP server port
EXPOSE 8081

# Run the server on the team's claimed port (8081)
# To override: docker run -p 8081:8081 coil-c2gui WebServer.exe 8081
CMD ["WebServer.exe", "8081"]
