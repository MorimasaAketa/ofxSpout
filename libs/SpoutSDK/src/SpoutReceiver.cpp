﻿//
//		SpoutReceiver
//
//		Wrapper class so that a receiver object can be created independent of a sender
//
// ====================================================================================
//		Revisions :
//
//		27-07-14	- CreateReceiver - bUseActive flag instead of null name
//		03.09.14	- Cleanup
//		23.09.14	- return DirectX 11 capability in SetDX9
//		28.09.14	- Added Host FBO for ReceiveTexture
//		12.10.14	- changed SelectSenderPanel arg to const char
//		23.12.14	- added host fbo arg to ReceiveImage
//		08.02.15	- Changed default texture format for ReceiveImage in header to GL_RGBA
//		29.05.15	- Included SetAdapter for multiple adapters - Franz Hildgen.
//		02.06.15	- Added GetAdapter, GetNumAdapters, GetAdapterName
//		24.08.15	- Added GetHostPath to retrieve the path of the host that produced the sender
//		15.09.15	- Removed SetMemoryShareMode for 2.005 - now done globally by SpoutDirectX.exe
//		10.10.15	- Added transition flag to set invert true for 2.004 rather than default false for 2.005
//					- currently not used - see SpoutSDK.cpp CreateSender
//		14.11.15	- changed functions to "const char *" where required
//		18.11.15	- added CheckReceiver so that DrawSharedTexture can be used by a receiver
//		18.06.16	- Add invert to ReceiveImage
//		17.09.16	- removed CheckSpout2004() from constructor
//		13.01.17	- Add SetCPUmode, GetCPUmode, SetBufferMode, GetBufferMode
//					- Add HostFBO arg to DrawSharedTexture
//		15.01.17	- Add GetShareMode, SetShareMode
//		06.06.17	- Add OpenSpout
//		05.11.18	- Add IsSpoutInitialized
//		11.11.18	- Add 2.007 high level application functions
//		13.11.18	- Remove SetCPUmode, GetCPUmode
//		24.11.18	- Remove redundant GetImageSize
//		28.11.18	- Add IsFrameNew
//		11.12.18	- Add utility functions
//		05.01.19	- Make names for 2.007 functions compatible with SpoutLibrary
//		16.01.19	- Initialize class variables
//		16.03.19	- Add IsFrameCountEnabled
//		19.03.19	- Change IsInitialized to IsConnected
//		05.04.19	- Change GetSenderName(index, ..) to GetSender
//					  Create const char * GetSenderName for receiver class
//
// ====================================================================================
/*
	Copyright (c) 2014-2019, Lynn Jarvis. All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, 
	are permitted provided that the following conditions are met:

		1. Redistributions of source code must retain the above copyright notice, 
		   this list of conditions and the following disclaimer.

		2. Redistributions in binary form must reproduce the above copyright notice, 
		   this list of conditions and the following disclaimer in the documentation 
		   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"	AND ANY 
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE	ARE DISCLAIMED. 
	IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "SpoutReceiver.h"

SpoutReceiver::SpoutReceiver()
{
	m_SenderNameSetup[0] = 0;
	m_SenderName[0] = 0;
	m_TextureID = 0;
	m_TextureTarget = 0;
	m_bInvert = false;
	m_bUseActive = false;
	m_Width = 0;
	m_Height = 0;
	m_bUpdate = false;
	m_bConnected = false;
}

//---------------------------------------------------------
SpoutReceiver::~SpoutReceiver()
{
	CloseReceiver();
}

// ================= 2.007 functions ======================


//---------------------------------------------------------
void SpoutReceiver::SetupReceiver(unsigned int width, unsigned int height, bool bInvert)
{
	// CreateReceiver will use the active sender unless the user 
	// has specified a sender to connect to using SetReceiverName
	m_SenderNameSetup[0] = 0;
	m_SenderName[0] = 0;
	m_bUseActive = true;

	// Record details for subsequent functions
	m_Width = width;
	m_Height = height;
	m_bInvert = bInvert; // Default false
	m_bUpdate = false;
	m_bConnected = false;

}

//---------------------------------------------------------
void SpoutReceiver::SetReceiverName(const char * SenderName)
{
	if (SenderName && SenderName[0]) {
		strcpy_s(m_SenderNameSetup, 256, SenderName);
		strcpy_s(m_SenderName, 256, SenderName);
		m_bUseActive = false; // the user has specified a sender to connect to
	}
}

//---------------------------------------------------------
bool SpoutReceiver::ReceiveTextureData(GLuint TextureID, GLuint TextureTarget, GLuint HostFbo)
{
	m_bUpdate = false;

	// Initialization is recorded in the spout class for sender or receiver
	if (!IsConnected()) {
		if (CreateReceiver(m_SenderName, m_Width, m_Height, m_bUseActive)) {
			// Signal the application to update the receiving texture size
			// Retrieved with a call to the Updated function
			m_bUpdate = true;
			m_bConnected = true;
			return true;
		}
	}
	else {
		// Save sender name and dimensions to test for change
		char name[256];
		strcpy_s(name, 256, m_SenderName);
		unsigned int width = m_Width;
		unsigned int height = m_Height;
		// Receive a shared texture but don't read it into the user texture yet
		if (ReceiveTexture(name, width, height)) {
			// Test for sender name or size change
			if (width != m_Width
				|| height != m_Height
				|| strcmp(name, m_SenderName) != 0) {
				// Update name
				strcpy_s(m_SenderName, 256, name);
				// Update class dimensions
				m_Width = width;
				m_Height = height;
				// Signal the application to update the receiving texture
				m_bUpdate = true;
				return true;
			}
			else {
				// Read the shared texture to the user texture
				return spout.interop.ReadTexture(m_SenderName, TextureID, TextureTarget, m_Width, m_Height, false, HostFbo);
			}
		}
		else {
			// receiving failed
			CloseReceiver();
			return false;
		}
	}

	// No connection
	return false;

}

//---------------------------------------------------------
bool SpoutReceiver::ReceiveImageData(unsigned char *pixels, GLenum glFormat, GLuint HostFbo)
{
	m_bUpdate = false;

	if (!IsConnected()) {
		if (CreateReceiver(m_SenderName, m_Width, m_Height, m_bUseActive)) {
			m_bUpdate = true;
			m_bConnected = true;
			return true;
		}
	}
	else {
		char sendername[256];
		strcpy_s(sendername, 256, m_SenderName);
		unsigned int width = m_Width;
		unsigned int height = m_Height;
		// Receive a shared image but don't read it into the user pixels yet
		if (ReceiveImage(sendername, width, height, NULL)) {
			// Test for sender name or size change
			if (width != m_Width
				|| height != m_Height
				|| strcmp(m_SenderName, sendername) != 0) {
				// Update the connected sender name
				strcpy_s(m_SenderName, 256, sendername);
				// Update class dimensions
				m_Width = width;
				m_Height = height;
				// Signal the application to update the receiving pixels
				m_bUpdate = true;
				return true;
			}
			else {
				// Read the shared texture or memory directly into the pixel buffer
				// Copy functions handle the formats supported
				return spout.interop.ReadTexturePixels(m_SenderName, pixels, width, height, glFormat, m_bInvert, HostFbo);
			}
		}
		else {
			// receiving failed
			CloseReceiver();
			return false;
		}
	}

	// No connection
	return false;

}

//---------------------------------------------------------
bool SpoutReceiver::IsUpdated()
{
	// Return whether the application texture needs updating.
	// The application must update the receiving texture before
	// the next call to ReceiveTextureData when the update flag is reset.
	return m_bUpdate;
}

//---------------------------------------------------------
bool SpoutReceiver::IsConnected()
{
	return m_bConnected;
}

//---------------------------------------------------------
void SpoutReceiver::CloseReceiver()
{
	ReleaseReceiver();
	// Restore the sender name that the user specified in SetupReceiver
	strcpy_s(m_SenderName, 256, m_SenderNameSetup);
	m_Width = 0;
	m_Height = 0;
	m_bUpdate = false;
	m_bConnected = false;
}

//---------------------------------------------------------
void SpoutReceiver::SelectSender()
{
	SelectSenderPanel();
}

//---------------------------------------------------------
const char * SpoutReceiver::GetSenderName()
{
	return m_SenderName;
}

//---------------------------------------------------------
unsigned int SpoutReceiver::GetSenderWidth()
{
	return m_Width;
}

//---------------------------------------------------------
unsigned int SpoutReceiver::GetSenderHeight()
{
	return m_Height;
}

//---------------------------------------------------------
double SpoutReceiver::GetSenderFps()
{
	return spout.interop.frame.GetSenderFps();
}

//---------------------------------------------------------
long SpoutReceiver::GetSenderFrame()
{
	return spout.interop.frame.GetSenderFrame();
}

//---------------------------------------------------------
bool SpoutReceiver::IsFrameNew()
{
	return spout.interop.frame.IsFrameNew();
}

//---------------------------------------------------------
void SpoutReceiver::DisableFrameCount()
{
	spout.interop.frame.DisableFrameCount();
}

//---------------------------------------------------------
bool SpoutReceiver::IsFrameCountEnabled()
{
	return spout.interop.frame.IsFrameCountEnabled();
}

// ================= end 2.007 functions ===================


//---------------------------------------------------------
bool SpoutReceiver::OpenSpout()
{
	return spout.OpenSpout();
}

//---------------------------------------------------------
bool SpoutReceiver::CreateReceiver(char* name, unsigned int &width, unsigned int &height, bool bUseActive)
{
	return spout.CreateReceiver(name, width, height, bUseActive);
}

//---------------------------------------------------------
void SpoutReceiver::ReleaseReceiver()
{
	spout.ReleaseReceiver();
}

//---------------------------------------------------------
bool SpoutReceiver::ReceiveTexture(char* name, unsigned int &width, unsigned int &height, GLuint TextureID, GLuint TextureTarget, bool bInvert, GLuint HostFBO)
{
	return spout.ReceiveTexture(name, width, height, TextureID, TextureTarget, bInvert, HostFBO);
}

#ifdef legacyOpenGL
//---------------------------------------------------------
bool SpoutReceiver::DrawSharedTexture(float max_x, float max_y, float aspect, bool bInvert, GLuint HostFBO)
{
	return spout.DrawSharedTexture(max_x, max_y, aspect, bInvert, HostFBO);
}
#endif

//---------------------------------------------------------
bool SpoutReceiver::ReceiveImage(char* Sendername, 
								 unsigned int &width, 
								 unsigned int &height, 
								 unsigned char* pixels, 
								 GLenum glFormat, 
								 bool bInvert,
								 GLuint HostFBO)
{
	return spout.ReceiveImage(Sendername, width, height, pixels, glFormat, bInvert, HostFBO);
}

//---------------------------------------------------------
void SpoutReceiver::RemovePadding(const unsigned char *source, unsigned char *dest,
	unsigned int width, unsigned int height, unsigned int stride, GLenum glFormat)
{
	return spout.RemovePadding(source, dest, width, height, stride, glFormat);
}

//---------------------------------------------------------
bool SpoutReceiver::CheckReceiver(char* name, unsigned int &width, unsigned int &height, bool &bConnected)
{
	return spout.CheckReceiver(name, width, height, bConnected);
}

//---------------------------------------------------------
bool SpoutReceiver::SelectSenderPanel(const char* message)
{
	return spout.SelectSenderPanel(message);
}

//---------------------------------------------------------
bool SpoutReceiver::BindSharedTexture()
{
	return spout.BindSharedTexture();
}

//---------------------------------------------------------
bool SpoutReceiver::UnBindSharedTexture()
{
	return spout.UnBindSharedTexture();
}

//---------------------------------------------------------
int  SpoutReceiver::GetSenderCount()
{
	return spout.GetSenderCount();
}

//---------------------------------------------------------
bool SpoutReceiver::GetSender(int index, char* sendername, int MaxNameSize)
{
	return spout.GetSender(index, sendername, MaxNameSize);
}

//---------------------------------------------------------
bool SpoutReceiver::GetSenderInfo(const char* sendername, unsigned int &width, unsigned int &height, HANDLE &dxShareHandle, DWORD &dwFormat)
{
	return spout.GetSenderInfo(sendername, width, height, dxShareHandle, dwFormat);
}

//---------------------------------------------------------
bool SpoutReceiver::GetActiveSender(char* Sendername)
{
	return spout.GetActiveSender(Sendername);
}

//---------------------------------------------------------
bool SpoutReceiver::SetActiveSender(const char* Sendername)
{
	return spout.SetActiveSender(Sendername);
}

//---------------------------------------------------------
bool SpoutReceiver::GetMemoryShareMode()
{
	return spout.GetMemoryShareMode();
}

//---------------------------------------------------------
bool SpoutReceiver::SetMemoryShareMode(bool bMem)
{
	return spout.SetMemoryShareMode(bMem);
}

//---------------------------------------------------------
int SpoutReceiver::GetShareMode()
{
	return spout.GetShareMode();
}

//---------------------------------------------------------
bool SpoutReceiver::SetShareMode(int mode)
{
	return (spout.SetShareMode(mode));
}

//---------------------------------------------------------
bool SpoutReceiver::GetBufferMode()
{
	return spout.GetBufferMode();
}

//---------------------------------------------------------
void SpoutReceiver::SetBufferMode(bool bActive)
{
	spout.SetBufferMode(bActive);
}

//---------------------------------------------------------
bool SpoutReceiver::GetDX9()
{
	return spout.interop.isDX9();
}

//---------------------------------------------------------
bool SpoutReceiver::SetDX9(bool bDX9)
{
	return spout.interop.UseDX9(bDX9);
}

//---------------------------------------------------------
bool SpoutReceiver::GetDX9compatible()
{
	if (spout.interop.DX11format == DXGI_FORMAT_B8G8R8A8_UNORM)
		return true;
	else
		return false;
}

//---------------------------------------------------------
void SpoutReceiver::SetDX9compatible(bool bCompatible)
{
	if(bCompatible) {
		// DX11 -> DX9 only works if the DX11 format is set to DXGI_FORMAT_B8G8R8A8_UNORM
		spout.interop.SetDX11format(DXGI_FORMAT_B8G8R8A8_UNORM);
	}
	else {
		// DX11 -> DX11 only
		spout.interop.SetDX11format(DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}

//---------------------------------------------------------
int SpoutReceiver::GetAdapter()
{
	return spout.GetAdapter();
}

//---------------------------------------------------------
bool SpoutReceiver::SetAdapter(int index)
{
	return spout.SetAdapter(index);
}

//---------------------------------------------------------
int SpoutReceiver::GetNumAdapters()
{
	return spout.GetNumAdapters();
}

//---------------------------------------------------------
// Get an adapter name
bool SpoutReceiver::GetAdapterName(int index, char* adaptername, int maxchars)
{
	return spout.GetAdapterName(index, adaptername, maxchars);
}

//---------------------------------------------------------
int SpoutReceiver::GetMaxSenders()
{
	// Get the maximum senders allowed from the sendernames class
	return(spout.interop.senders.GetMaxSenders());
}

//---------------------------------------------------------
void SpoutReceiver::SetMaxSenders(int maxSenders)
{
	// Sets the maximum senders allowed
	spout.interop.senders.SetMaxSenders(maxSenders);
}

//---------------------------------------------------------
bool SpoutReceiver::GetHostPath(const char* sendername, char* hostpath, int maxchars)
{
	return spout.GetHostPath(sendername, hostpath, maxchars);
}

//---------------------------------------------------------
int SpoutReceiver::GetVerticalSync()
{
	return spout.interop.GetVerticalSync();
}

//---------------------------------------------------------
bool SpoutReceiver::SetVerticalSync(bool bSync)
{
	return spout.interop.SetVerticalSync(bSync);
}






