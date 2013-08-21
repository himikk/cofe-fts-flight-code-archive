#pragma once

#include "ComPort.h"
#include "DataSource.h"
#include "internalTime.h"

#include <string>
#include <iostream>

using std::cin;

namespace UCSB_Telescope
{

using nsDataSource::string;
using nsDataSource::strings;
using nsDataSource::FieldIter;

// Dictionary Data

class Telescope : public nsDataSource::DataSource, public nsDataSource::AutoRegister<Telescope>
{
public:
	Telescope(void);
	virtual ~Telescope(void);

public :
	virtual bool Start()
	{
		GetDeviceHolder().RegisterWriter(*this, GetClassGUID());
		
		return m_port.Start();
	}
	
	virtual bool AddDeviceTypes()
	{
		GetDeviceHolder().AddDeviceType(*this);
		return nsDataSource::DataSource::AddDeviceTypes();
	}
	
	virtual bool TickImpl(InternalTime::internalTime const& now)
	{
		m_current = now;
		
		char buffer[512];
		DWORD read = 0;
		

		if (m_port.Read(buffer, &read))
		{
			UCSBUtility::AddData(m_buffer, buffer, read);
			InterpretBuffer();
		}
		
		return true;
	}
	
	std::string::size_type Read(std::string::size_type offset)
	{
		char jnk[sizeof(localData)];
		for(int i = 0; i < sizeof(localData); i++,offset++)
			jnk[i] = m_buffer[offset];
		/*std::string::size_type end = m_buffer.find_first_not_of("0123456789-+.e", offset);
        if (end == std::string::npos)
        {
            // malformed string, don't parse anything
            return end;
        }

        std::string value(m_buffer.begin() + offset, m_buffer.begin() + end);

        destination = static_cast<float>(atof(value.c_str()));*/
		memcpy(&m_Data,jnk,sizeof(localData));
        return offset;
	}
		
	/*std::string::size_type Parse(std::string::size_type offset)
	{
		int ignore = 0;
		while (offset != std::string::npos)
        {     			
            switch (ignore)
            {
            case 0 :
                offset = Read(m_Data.rev, offset);
                break;

            case 1 :
                offset = Read(m_Data.channel00t, offset);
				offset = Read(m_Data.channel00q, offset);
				offset = Read(m_Data.channel00u, offset);
                break;

            case 2 :
                offset = Read(m_Data.channel01t, offset);
				offset = Read(m_Data.channel01q, offset);
				offset = Read(m_Data.channel01u, offset);
                break;
			
			case 3 :
                offset = Read(m_Data.channel02t, offset);
				offset = Read(m_Data.channel02q, offset);
				offset = Read(m_Data.channel02u, offset);
                break;

			case 4 :
                offset = Read(m_Data.channel03t, offset);
				offset = Read(m_Data.channel03q, offset);
				offset = Read(m_Data.channel03u, offset);
                break;
			
			case 5 :
                offset = Read(m_Data.channel04t, offset);
				offset = Read(m_Data.channel04q, offset);
				offset = Read(m_Data.channel04u, offset);
                break;

			case 6 :
                offset = Read(m_Data.channel05t, offset);
				offset = Read(m_Data.channel05q, offset);
				offset = Read(m_Data.channel05u, offset);
                break;

			case 7 :
                offset = Read(m_Data.channel06t, offset);
				offset = Read(m_Data.channel06q, offset);
				offset = Read(m_Data.channel06u, offset);
                break;

			case 8 :
                offset = Read(m_Data.channel07t, offset);
				offset = Read(m_Data.channel07q, offset);
				offset = Read(m_Data.channel07u, offset);
                break;

			case 9 :
                offset = Read(m_Data.channel08t, offset);
				offset = Read(m_Data.channel08q, offset);
				offset = Read(m_Data.channel08u, offset);
                break;

			case 10 :
                offset = Read(m_Data.channel09t, offset);
				offset = Read(m_Data.channel09q, offset);
				offset = Read(m_Data.channel09u, offset);
                break;

			case 11 :
                offset = Read(m_Data.channel10t, offset);
				offset = Read(m_Data.channel10q, offset);
				offset = Read(m_Data.channel10u, offset);
                break;

			case 12 :
                offset = Read(m_Data.channel11t, offset);
				offset = Read(m_Data.channel11q, offset);
				offset = Read(m_Data.channel11u, offset);
                break;

			case 13 :
                offset = Read(m_Data.channel12t, offset);
				offset = Read(m_Data.channel12q, offset);
				offset = Read(m_Data.channel12u, offset);
                break;

			case 14 :
                offset = Read(m_Data.channel13t, offset);
				offset = Read(m_Data.channel13q, offset);
				offset = Read(m_Data.channel13u, offset);
                break;

			case 15 :
                offset = Read(m_Data.channel14t, offset);
				offset = Read(m_Data.channel14q, offset);
				offset = Read(m_Data.channel14u, offset);
                break;

			case 16 :
                offset = Read(m_Data.channel15t, offset);
				offset = Read(m_Data.channel15q, offset);
				offset = Read(m_Data.channel15u, offset);
                break;

            default :
                return offset;
            }
			ignore++;
		}
		return offset;
	}*/
	
	void InterpretBuffer()
	{
		std::string::size_type start = 0;

        while (start != std::string::npos)
        {
            // Find the next '$'
            start = m_buffer.find('$', start);

            // If the buffer is invalid, clear it
            if (start == std::string::npos)
            {
                m_buffer.clear();
                return;
            }

            // If we don't have a whole message, return
            std::string::size_type last = m_buffer.find('*', start + 1);
			while( (last - start - 1) != sizeof(localData))
			{
				last = m_buffer.find('*', last + 1);
				if( last == std::string::npos)
					start = m_buffer.find('$', start+1);
				if (start == std::string::npos)
				{
					m_buffer.clear();
					return;
				}
			}
            /*if (last == std::string::npos)
            {
                // if we have never read any data, no need to adjust the buffer
                if (start == 0)
                {
                    return;
                }

                m_buffer = std::string(m_buffer.begin() + start, m_buffer.end());
                return;
            }*/

            start = Read(++start);
            GetDeviceHolder().WriteDataWithCache(m_current, *this, m_Data);
        }
	}
	
	virtual FieldIter Configure(FieldIter beg, FieldIter end)
	{
		const DWORD paramCount = 1;
		// read in the parameters for Telescope
		if ((end - beg) < paramCount)
		{
			UCSBUtility::LogError(__FUNCTION__, __FILE__, __LINE__, "%d parameters expected"
				", %d parameters found\n", paramCount, (end - beg));
			return end;
		}

		// Read the frequency
		SetFrequency(UCSBUtility::ToINT<DWORD>(*beg++));
		
		return m_port.Configure(beg, end);
	}
	
public :
	static std::string GetName()
	{
		return "Telescope";
	}
	
	static std::vector<nsDataSource::ChannelDefinition>  GetClassDescription()
	{
		return localData::GetClassDescription();
	}
	
	static GUID GetClassGUID()
	{
		return localData::GetClassGUID();
	}
	
private :
	/*union btod
	{
		double value;
		char buffer[sizeof(double)];
	};*/

	struct localData
	{
		float	rev;
	    float   channel00t;
        float   channel00q;
        float   channel00u;
	    float   channel01t;
        float   channel01q;
        float   channel01u;
		float   channel02t;
        float   channel02q;
        float   channel02u;
		float   channel03t;
        float   channel03q;
        float   channel03u;
		float   channel04t;
        float   channel04q;
        float   channel04u;
		float   channel05t;
        float   channel05q;
        float   channel05u;
		float   channel06t;
        float   channel06q;
        float   channel06u;
		float   channel07t;
        float   channel07q;
        float   channel07u;
		float   channel08t;
        float   channel08q;
        float   channel08u;
		float   channel09t;
        float   channel09q;
        float   channel09u;

		float   channel10t;
        float   channel10q;
        float   channel10u;
		float   channel11t;
        float   channel11q;
        float   channel11u;
		float   channel12t;
        float   channel12q;
        float   channel12u;
		float   channel13t;
        float   channel13q;
        float   channel13u;
		float   channel14t;
        float   channel14q;
        float   channel14u;
		float   channel15t;
        float   channel15q;
        float   channel15u;

		static GUID GetClassGUID()
		{
			// {A38A47EE-D74E-479a-A2C1-FE7EFDC2C4CF}
			static const GUID classGUID = 
			{ 0xa38a47ee, 0xd74e, 0x479a, { 0xa2, 0xc1, 0xfe, 0x7e, 0xfd, 0xc2, 0xc4, 0xcf } };

			return classGUID;
		}
		
		static std::vector<nsDataSource::ChannelDefinition>  GetClassDescription()
		{
			static nsDataSource::ChannelDefinition definition[] = 
			{
				{"SPFLOAT_I", "Rev"},
                {"SPFLOAT_I", "Channel 00T"},
				{"SPFLOAT_I", "Channel 00Q"},
				{"SPFLOAT_I", "Channel 00U"},
				{"SPFLOAT_I", "Channel 01T"},
				{"SPFLOAT_I", "Channel 01Q"},
				{"SPFLOAT_I", "Channel 01U"},
				{"SPFLOAT_I", "Channel 02T"},
				{"SPFLOAT_I", "Channel 02Q"},
				{"SPFLOAT_I", "Channel 02U"},
				{"SPFLOAT_I", "Channel 03T"},
				{"SPFLOAT_I", "Channel 03Q"},
				{"SPFLOAT_I", "Channel 03U"},
				{"SPFLOAT_I", "Channel 04T"},
				{"SPFLOAT_I", "Channel 04Q"},
				{"SPFLOAT_I", "Channel 04U"},
				{"SPFLOAT_I", "Channel 05T"},
				{"SPFLOAT_I", "Channel 05Q"},
				{"SPFLOAT_I", "Channel 05U"},
				{"SPFLOAT_I", "Channel 06T"},
				{"SPFLOAT_I", "Channel 06Q"},
				{"SPFLOAT_I", "Channel 06U"},
				{"SPFLOAT_I", "Channel 07T"},
				{"SPFLOAT_I", "Channel 07Q"},
				{"SPFLOAT_I", "Channel 07U"},
				{"SPFLOAT_I", "Channel 08T"},
				{"SPFLOAT_I", "Channel 08Q"},
				{"SPFLOAT_I", "Channel 08U"},
				{"SPFLOAT_I", "Channel 09T"},
				{"SPFLOAT_I", "Channel 09Q"},
				{"SPFLOAT_I", "Channel 09U"},
				{"SPFLOAT_I", "Channel 10T"},
				{"SPFLOAT_I", "Channel 10Q"},
				{"SPFLOAT_I", "Channel 10U"},
				{"SPFLOAT_I", "Channel 11T"},
				{"SPFLOAT_I", "Channel 11Q"},
				{"SPFLOAT_I", "Channel 11U"},
				{"SPFLOAT_I", "Channel 12T"},
				{"SPFLOAT_I", "Channel 12Q"},
				{"SPFLOAT_I", "Channel 12U"},
				{"SPFLOAT_I", "Channel 13T"},
				{"SPFLOAT_I", "Channel 13Q"},
				{"SPFLOAT_I", "Channel 13U"},
				{"SPFLOAT_I", "Channel 14T"},
				{"SPFLOAT_I", "Channel 14Q"},
				{"SPFLOAT_I", "Channel 14U"},
				{"SPFLOAT_I", "Channel 15T"},
				{"SPFLOAT_I", "Channel 15Q"},
				{"SPFLOAT_I", "Channel 15U"},
            };

            return UCSBUtility::ConvertToVector(definition);
        }
	};

	localData					m_Data;
	ComPort						m_port;
	std::string					m_buffer;
	InternalTime::internalTime	m_current;
};

}