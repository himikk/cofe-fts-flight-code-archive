#pragma once

#include <DataSource.h>
#include "internalTime.h"
#include "cbw.h"

#include <string>

namespace UCSB_MMCSource
{

using nsDataSource::FieldIter;

class MMCSource : public nsDataSource::DataSource, public nsDataSource::AutoRegister<MMCSource>
{

public:
    MMCSource(void);
    ~MMCSource(void);

    virtual bool TickImpl(InternalTime::internalTime const& now);

    virtual bool AddDeviceTypes()
    {
        GetDeviceHolder().AddDeviceType(*this);
        return nsDataSource::DataSource::AddDeviceTypes();
    }
    
    static std::string GetName()
    {
        return "MMC Device";
    }

    virtual FieldIter Configure(FieldIter beg, FieldIter end)
    {
        const DWORD paramCount = 2;
        // read in the parameters for CounterSource
        if ((end - beg) < paramCount)
        {
            UCSBUtility::LogError(__FUNCTION__, __FILE__, __LINE__, "%d parameters expected"
                ", %d parameters found\n", paramCount, (end - beg));
            return end;
        }

        // Read the board number from the configuration
        std::string boardNumString = *beg++;
        m_boardNum = atoi(boardNumString.c_str());

        // Read the frequency from the configuration
        std::string frequency = *beg++;

        SetFrequency(static_cast<DWORD>(atoi(frequency.c_str())));

        return beg;
    }

    virtual bool Start();

    static std::vector<nsDataSource::ChannelDefinition>  GetClassDescription()
    {
        return localData::GetClassDescription();
    }

    static GUID GetClassGUID()
    {
        return localData::GetClassGUID();
    }

    struct localData
    {
        DWORD   value;

        static GUID GetClassGUID()
        {
            // {83DE51AC-7648-466a-9ED4-6B140977C4AC}
            static const GUID classGUID = 
            { 0x83de51ac, 0x7648, 0x466a, { 0x9e, 0xd4, 0x6b, 0x14, 0x9, 0x77, 0xc4, 0xac } };

            return classGUID;
        }

        static std::vector<nsDataSource::ChannelDefinition>  GetClassDescription()
        {
            static nsDataSource::ChannelDefinition definition[] =
            {
                {"ULONG_I", "value"},
            };

            return UCSBUtility::ConvertToVector(definition);
        }
    };

private :
    // Data used to communicate data from the board to the spaceball files
    localData   ld;

    // Data used internally

    // Read from Devices.cfg
    int m_boardNum;

    // Last error value
    int m_ulStat;
};
}