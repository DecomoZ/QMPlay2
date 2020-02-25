/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <VideoFilter.hpp>

#include <QDebug>

VideoFilter::VideoFilter(bool fillDefaultSupportedPixelFormats)
{
    if (!fillDefaultSupportedPixelFormats)
        return;

    m_supportedPixelFormats = {
        AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUVJ420P,
        AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUVJ422P,
        AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_YUVJ444P,
        AV_PIX_FMT_YUV410P,
        AV_PIX_FMT_YUV411P,
        AV_PIX_FMT_YUVJ411P,
        AV_PIX_FMT_YUV440P,
        AV_PIX_FMT_YUVJ440P,
    };
}
VideoFilter::~VideoFilter()
{
}

void VideoFilter::clearBuffer()
{
    m_internalQueue.clear();

    m_secondFrame = false;
    m_lastTS = qQNaN();
}

bool VideoFilter::removeLastFromInternalBuffer()
{
    if (!m_internalQueue.isEmpty())
    {
        m_internalQueue.removeLast();
        return true;
    }
    return false;
}

void VideoFilter::processParamsDeint()
{
    m_secondFrame = false;
    m_lastTS = qQNaN();

    m_deintFlags = getParam("DeinterlaceFlags").toInt();
}

void VideoFilter::addFramesToInternalQueue(QQueue<Frame> &framesQueue)
{
    while (!framesQueue.isEmpty())
    {
        const Frame &videoFrame = framesQueue.constFirst();
        if (videoFrame.isEmpty() || !m_supportedPixelFormats.contains(videoFrame.pixelFormat()))
            break;
        m_internalQueue.enqueue(framesQueue.dequeue());
    }
}
void VideoFilter::addFramesToDeinterlace(QQueue<Frame> &framesQueue)
{
    while (!framesQueue.isEmpty())
    {
        const Frame &videoFrame = framesQueue.constFirst();
        if (videoFrame.isEmpty() || !m_supportedPixelFormats.contains(videoFrame.pixelFormat()))
            break;
        if ((m_deintFlags & AutoDeinterlace) && !videoFrame.isInterlaced())
            break;
        m_internalQueue.enqueue(framesQueue.dequeue());
    }
}

void VideoFilter::deinterlaceDoublerCommon(Frame &frame)
{
    const double initialTS = frame.ts();

    if (m_secondFrame)
    {
        frame.setTS(getMidFrameTS(frame.ts(), m_lastTS));
        frame.setIsSecondField(true);
        m_internalQueue.removeFirst();
    }

    if (m_secondFrame || qIsNaN(m_lastTS))
        m_lastTS = initialTS;

    m_secondFrame = !m_secondFrame;
}

bool VideoFilter::isTopFieldFirst(const Frame &videoFrame) const
{
    return ((m_deintFlags & AutoParity) && videoFrame.isInterlaced())
        ? videoFrame.isTopFieldFirst()
        : (m_deintFlags & TopFieldFirst)
    ;
}

double VideoFilter::getMidFrameTS(double ts1, double ts2) const
{
    return ts1 + qAbs(ts1 - ts2) / 2.0;
}
