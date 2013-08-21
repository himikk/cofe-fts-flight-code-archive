#include "StdAfx.h"
#include "MMCSource.h"

#pragma comment(lib, "cbw32.lib")


namespace UCSB_MMCSource
{

/* Variable Declarations */
static const int Direction = DIGITALIN;
static const int g_ports[] =
{
    FIRSTPORTC, // By doing this in reverse order
    FIRSTPORTB, // we get simpler math for the 24-bit value
    FIRSTPORTA, // (((C << 8) + B) << 8) + A
                // This may not look simpler, but loops better
                // And it ensures we don't have any overflow problems
                // Since the value we are shifting is 32 bits, not 16
                // Note: a << 8 is the same as a * 256
                // Also: << has low precedence, so don't do a << b + c
                //  it does a << (b + c)
};

MMCSource::MMCSource(void) 
    : AutoRegister<MMCSource>(0)
{
    float  RevLevel = static_cast<float>(CURRENTREVNUM);

    // This is a little weird
    // I don't understand why this takes a non-const pointer
    // probably an artifact and doesn't change the value
    m_ulStat = cbDeclareRevision(&RevLevel);
}

MMCSource::~MMCSource(void)
{
}

bool MMCSource::Start()
{
    for (int i=0; i < ARRAYSIZE(g_ports); ++i)
    {
        m_ulStat = cbDConfigPort (m_boardNum, g_ports[i], Direction);

        if (m_ulStat != 0)
        {
            UCSBUtility::LogError(__FUNCTION__, __FILE__, __LINE__, 
                "Unable to intialize MMC board #%d : port index #%d", 
                m_boardNum, i);

            return false;
        }

    }

    GetDeviceHolder().RegisterWriter(*this, GetClassGUID());
    return true;
}

bool MMCSource::TickImpl(InternalTime::internalTime const& now)
{
    // Re-intialize our data value
    ld.value = 0;

    for (int i=0; i < ARRAYSIZE(g_ports); ++i)
    {
        WORD DataValue = 0;
		m_ulStat = cbDIn(m_boardNum, g_ports[i], &DataValue);
        
        // If any read fails, inform the system and quit before writing data
        if (m_ulStat != 0)
        {
            UCSBUtility::LogError(__FUNCTION__, __FILE__, __LINE__, 
                "Unable to read MMC board #%d : port index #%d", 
                m_boardNum, i);
            
            return false;
        }

        // 
        ld.value = (ld.value << 8) + DataValue;
    }

    // Copy the total 
    
    GetDeviceHolder().WriteDataWithCache(now, *this, ld);
    return true;
}

}