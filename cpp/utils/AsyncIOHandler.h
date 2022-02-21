
#pragma once

#include <string>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <aio.h>
#include <string.h>
#include <assert.h>
#include <memory>
#include <sys/stat.h>

#include "SDLWrapper.h"

//
enum class AsyncIOMode
{
    READ = 0,
    WRITE
};

// data type of data generation
//typedef uint32_t sim_data_t;
typedef float sim_data_t;

// for IntelliSense
class AsyncIOHandler;
static inline std::shared_ptr<AsyncIOHandler> newAsyncIOHandler(size_t _data_block_size, 
                                                                AsyncIOMode _mode=AsyncIOMode::READ,
                                                                const std::string& _file_name="/home/iomanip/src/IABP_RL/data.bin")
{   return std::make_shared<AsyncIOHandler>(_data_block_size, _mode, _file_name);   }

//-----------------------------------------------------------------------------------
class AsyncIOHandler
{
private:
    //-----------------------------------------------------------------------------------
    int m_filePtr = -1;
    std::string m_fileName;
    AsyncIOMode m_mode;
    
    size_t m_dataBlockSize = 0;
    size_t m_dataSizeBytes = 0;
    sim_data_t* m_data = nullptr;

    size_t m_fileSz = 0;
    //-----------------------------------------------------------------------------------
public:
    //-----------------------------------------------------------------------------------
    const size_t getDataBlockSize() const { return m_dataBlockSize; }
    const size_t getDataSizeBytes() const { return m_dataSizeBytes; }
    //-----------------------------------------------------------------------------------
public:
    //-----------------------------------------------------------------------------------
    AsyncIOHandler(size_t _data_block_size,
                   AsyncIOMode _mode=AsyncIOMode::READ, 
                   const std::string& _file_name="/home/iomanip/src/IABP_RL/data.bin") :
        m_dataBlockSize(_data_block_size),
        m_mode(_mode),    
        m_fileName(_file_name)
    {
        m_dataSizeBytes = m_dataBlockSize * sizeof(sim_data_t);

        printf("FileIOHandler initialized in AsyncIOMode::%s mode (block size = %lu).\n", 
            m_mode==AsyncIOMode::READ ? "READ" : "WRITE",
            m_dataBlockSize);

        // open file for I/O
        if (m_mode == AsyncIOMode::READ)
        {
            m_filePtr = open(m_fileName.c_str(), O_RDONLY);
            validatePtr();
            printf("Data stream (%s) open (%d).\n", m_fileName.c_str(), m_filePtr);
            struct stat st;
            stat(m_fileName.c_str(), &st);
            m_fileSz = st.st_size;
            printf("Current stream size = %lu\n", m_fileSz);
        }
        else if (m_mode == AsyncIOMode::WRITE)
        {
            m_filePtr = open(m_fileName.c_str(), O_CREAT | O_RDWR | O_APPEND, 0666);
            validatePtr();
            printf("Data stream (%s) open (%d).\n", m_fileName.c_str(), m_filePtr);
        }
        else
        {
            printf("Unknown data stream access mode. Exit.\n");
            exit(-1);
        }

        // setup data pointer
        m_data = new sim_data_t(m_dataBlockSize);

    }
    //-----------------------------------------------------------------------------------
    ~AsyncIOHandler()
    {
        if (m_filePtr != -1)
        {
            printf("--- Closing data stream (%d) ---\n", m_filePtr);
            close(m_filePtr);
        }

        if (m_data != nullptr)
            delete[] m_data;
    }
    //-----------------------------------------------------------------------------------
    void aioWrite(const std::vector<sim_data_t>& _data)
    {
        memset(m_data, 0, sizeof(sim_data_t) * m_dataBlockSize);
        assert(_data.size() != m_dataBlockSize);

        // copy incoming (generated) data to data block
        memcpy(m_data, (void*)&_data[0], m_dataSizeBytes);

        // DEBUG
        //for (size_t i = 0; i < m_dataBlockSize; i++)
        //    printf("%.1f ", m_data[i]);
        //printf("\n");


        struct aiocb write_cb;
        memset(&write_cb, 0, sizeof(struct aiocb));
        write_cb.aio_fildes = m_filePtr;
        write_cb.aio_buf = (void*)m_data;
        write_cb.aio_nbytes = m_dataSizeBytes;
        //write_cb.aio_offset = (intptr_t)-1;   // already in O_APPEND mode : aio_write(3)
        write_cb.aio_reqprio = 0;
        write_cb.aio_sigevent.sigev_notify = SIGEV_NONE;

        if (aio_write(&write_cb) < 0)
            printf("Error: async I/O write failure.\n");

        //uint64_t t0 = SDL_GetPerformanceCounter();
        // wait for write to complete
        while (aio_error(&write_cb) == EINPROGRESS);
        //printf("%f\n", (float)(SDL_GetPerformanceCounter() - t0) / (float)SDL_GetPerformanceFrequency());

    }
    //-----------------------------------------------------------------------------------
    size_t aioRead(sim_data_t* _data_out)
    {
        // check if file size has increased
        struct stat st;
        stat(m_fileName.c_str(), &st);
        size_t file_sz_diff = st.st_size - m_fileSz;
        if (file_sz_diff > 0)
        {
            m_fileSz = st.st_size;
            memset(m_data, 0, m_dataSizeBytes);

            // read data to the m_data field
            struct aiocb read_cb;
            memset(&read_cb, 0, sizeof(struct aiocb));
            read_cb.aio_fildes = m_filePtr;
            read_cb.aio_offset = st.st_size - m_dataSizeBytes;
            read_cb.aio_buf = (void*)m_data;
            read_cb.aio_nbytes = m_dataSizeBytes;

            if (aio_read(&read_cb) < 0)
                printf("Error: async I/O read failure.\n");

            while (aio_error(&read_cb) == EINPROGRESS);

            // DEBUG
            for (int i = 0; i < m_dataBlockSize; i++)
                printf("%.1f ", m_data[i]);
            printf("\n");
        }

        // populate return vector
        memcpy(_data_out, m_data, m_dataSizeBytes);

        //
        return file_sz_diff;
    }

private:
    //-----------------------------------------------------------------------------------
    void validatePtr()
    {
        if (m_filePtr < 0)
        {
            printf("Bad file descriptor. Exit.\n");
            exit(-1);
        }
    }

};