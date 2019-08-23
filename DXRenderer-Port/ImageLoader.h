//*********************************************************
//
// ImageLoader
//
// Manages loading an image from disk or other stream source
// into Direct2D. Handles all codec operations (WIC),
// detecting image info, and providing a Direct2D ImageSource.
//
// ImageLoader relies on the caller to explicitly inform it
// of device lost/restored events, i.e. it does not
// independently register for IDeviceNotify.
//
// Throws WINCODEC_ERR_[foo] HRESULTs in exceptions as these
// match well with the intended error states.
//
//*********************************************************

#pragma once
#include "Common\DeviceResources.h"

#include <cstdarg>

namespace DXR
{
    /// <summary>
    /// State machine.
    /// </summary>
    /// <remarks>
    /// Valid transitions:
    /// NotInitialized      --> LoadingSucceeded || LoadingFailed
    /// LoadingFailed       --> [N/A]
    /// LoadingSucceeded    --> NeedDeviceResources
    /// NeedDeviceResources --> LoadingSucceeded
    /// </remarks>
    enum ImageLoaderState
    {
        NotInitialized,
        LoadingSucceeded,
        LoadingFailed,
        NeedDeviceResources // Device resources must be (re)created but otherwise image data is valid.
    };

    class ImageLoader
    {
    public:
        ImageLoader(const std::shared_ptr<DeviceResources>& deviceResources);
        ~ImageLoader();

        ImageLoaderState GetState() const { return m_state; };

        winrt::DXRenderer::ImageInfo LoadImageFromWic(_In_ IStream* imageStream);
        winrt::DXRenderer::ImageInfo LoadImageFromDirectXTex(std::wstring filename, std::wstring extension);

        ID2D1TransformedImageSource* GetLoadedImage(float zoom);
        ID2D1ColorContext* GetImageColorContext();
        winrt::DXRenderer::ImageInfo GetImageInfo();

        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();

    private:
        /// <summary>
        /// Throws if the internal ImageLoaderState does not match one of the valid values.
        /// Pass in one or more ImageLoaderState values.
        /// </summary>
        /// <param name="numStates">How many ImageLoaderState values are valid.</param>
        inline void EnforceStates(int numStates...)
        {
            va_list args;
            va_start(args, numStates);

            for (int i = 0; i < numStates; i++)
            {
                auto s = va_arg(args, ImageLoaderState);
                if (m_state == s) return;
            }

            winrt::throw_hresult(WINCODEC_ERR_WRONGSTATE);
        }

        void LoadImageCommon(_In_ IWICBitmapSource* source);
        void CreateDeviceDependentResourcesInternal();
        void PopulateImageInfoACKind(_Inout_ winrt::DXRenderer::ImageInfo* info, _In_ IWICBitmapSource* source);
        bool IsImageXboxHdrScreenshot(_In_ IWICBitmapSource* source);
        GUID TranslateDxgiFormatToWic(DXGI_FORMAT fmt);

        std::shared_ptr<DeviceResources>                m_deviceResources;

        // Device-independent
        winrt::com_ptr<IWICFormatConverter>             m_formatConvert;
        winrt::com_ptr<IWICColorContext>                m_wicColorContext;

        ImageLoaderState                                m_state;
        winrt::DXRenderer::ImageInfo                    m_imageInfo;

        // Device-dependent
        winrt::com_ptr<ID2D1ImageSourceFromWic>         m_imageSource;
        winrt::com_ptr<ID2D1ColorContext>               m_colorContext;
    };
}
