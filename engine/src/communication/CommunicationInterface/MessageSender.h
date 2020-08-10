#pragma once

#include "execution_graph/logic_controllers/CacheMachine.h"
#include "blazingdb/concurrency/BlazingThread.h"

using gpu_raw_buffer_container = blazingdb::transport::gpu_raw_buffer_container;

/*
*using gpu_raw_buffer_container = std::tuple<std::vector<std::size_t>, std::vector<const char *>,
											std::vector<ColumnTransport>,
											std::vector<std::unique_ptr<rmm::device_buffer>> >;

This is no good and needs to be replaced its so hard to tell whats going on in code if i have to see which part of the tuple has what
*/

/**
 * A Class that can be used to poll messages and then send them off.
 * Creating this class serves the purpose of allowing us to specify different combinations of serializers, conversion from buffer to frame combined with different methods for sending and addressing. 
 * @tparam SerializerFunction A function that can convert whatever we pull from Cache into metadata and buffers to be sent over some protocol
 * @tparam BufferToFrameFunction A function that can convert an rmm::device_buffer into a frame that can be sent over a protocol, is often a no op
 * @tparam NodeAddress NodeAddress will be used to give the SendingFunction a common api for getting a nodes address 
 * @tparam
 */
template <typename SerializerFunction, typename BufferToFrameFunction, typename NodeAddress, typename Sender >
class message_sender {
public:
    message_sender(std::shared_ptr<ral::cache::CacheMachine> output_cache, size_t num_threads, std::map<std::string, NodeAddress> node_address_map ) : output_cache{ output_cache}, pool{ num_threads}{

    }
    
private:
    ctpl::thread_pool<BlazingThread> pool; /**< The thread pool used by the the sender to send messages via some protocol */
    std::shared_ptr<ral::cache::CacheMachine> output_cache; /**< The thread pool used by the the sender to send messages via some protocol */
    std::map<std::string, NodeAddress> node_address_map; /**< A map of worker_id to NodeAddress */
    
    /**
     * A polling function that listens on a cache for data to exist and then sends it off via some protocol
     */
    void run_polling();
};


void message_sender::run_polling(){
    while(true){
        std::pair<std::unique_ptr<ral::frame::BlazingTable>,MetadataDictionary >  data_and_metadata = 
            std::unique_ptr<ral::cache::GPUCacheDataMetaData>(
                static_cast<ral::cache::GPUCacheDataMetaData*>(output_cache.pullCacheData().release())
                )->decacheWithMetaData();

        pool.push([message{move(data_and_metadata.first)},metadata{data_and_metadata.second}, node_address_map, output_cache] (int thread_id) {
            gpu_raw_buffer_container serialized SerializerFunction(std::move(message));
            //TODO: this needs to change we need to make the container a struct
            std::vector<std::size_t> buffer_sizes = std::get<0>(serialized);
            std::vector<const char *> buffers = std::get<1>(serialized);
            std::vector<ColumnTransport> column_transports = std::get<2>(serialized);
			std::vector<std::unique_ptr<rmm::device_buffer>> column_scopes std::get<3>(serialized);

            try{
                //Initializes the sender with information needed for communicating the function that begins transmission
                //This constructor will make a ucx call iwth all the metadata and wait for it to complete
                Sender sender(node_address_map,metadata, buffer_sizes, column_transports); 
                
                for(size_t buffer_index = 0; buffer_index < buffers.size(); buffer_index++){
                    sender.send(
                        BufferToFrameFunction(buffers[buffer_index]),
                        buffer_sizes[buffer_index],
                    );
                }

            }catch (ral::exception::communication_initialization e){

            }catch (ral::exception::communication_transmission e){
                
            }catch (std::exception e){

            }

        });

    }
}


template <typename NodeAddress>
class buffer_sender{
public:
    Sender(std::map<std::string, NodeAddress> node_address_map,metadata,buffer_sizes,column_transports){
        //iterate for workers this is destined for
        for( auto worker_id : StringUtil::split(metadata[WORKER_IDS_METADATA_LABEL],",")){
            if(node_address_map.find(worker_id) == node_address_map.end()){
                throw std::exception(); //TODO: make a real exception here
            }
            destinations.push_back(node_address_mapp[worker_id])
        }
    }
private:
    std::vector<NodeAddress> destinations;
}





#pragma once

#include "execution_graph/logic_controllers/CacheMachine.h"
#include "blazingdb/concurrency/BlazingThread.h"

using gpu_raw_buffer_container = blazingdb::transport::gpu_raw_buffer_container;

/*
*using gpu_raw_buffer_container = std::tuple<std::vector<std::size_t>, std::vector<const char *>,
											std::vector<ColumnTransport>,
											std::vector<std::unique_ptr<rmm::device_buffer>> >;

This is no good and needs to be replaced its so hard to tell whats going on in code if i have to see which part of the tuple has what
*/

/**
 * A Class that can be used to poll messages and then send them off.
 * Creating this class serves the purpose of allowing us to specify different combinations of serializers, conversion from buffer to frame combined with different methods for sending and addressing. 
 * @tparam SerializerFunction A function that can convert whatever we pull from Cache into metadata and buffers to be sent over some protocol
 * @tparam
 */
template <typename SerializerFunction, typename Sender >
class message_sender {
public:
    message_sender(std::shared_ptr<ral::cache::CacheMachine> output_cache, size_t num_threads, std::map<std::string, NodeAddress *> node_address_map ) : output_cache{ output_cache}, pool{ num_threads}{

    }
    
private:
    ctpl::thread_pool<BlazingThread> pool; /**< The thread pool used by the the sender to send messages via some protocol */
    std::shared_ptr<ral::cache::CacheMachine> output_cache; /**< The thread pool used by the the sender to send messages via some protocol */
    std::map<std::string, NodeAddress> node_address_map; /**< A map of worker_id to NodeAddress */
    
    /**
     * A polling function that listens on a cache for data to exist and then sends it off via some protocol
     */
    void run_polling();
};


void message_sender::run_polling(){
    while(true){
        std::pair<std::unique_ptr<ral::frame::BlazingTable>,MetadataDictionary >  data_and_metadata = 
            std::unique_ptr<ral::cache::GPUCacheDataMetaData>(
                static_cast<ral::cache::GPUCacheDataMetaData*>(output_cache.pullCacheData().release())
                )->decacheWithMetaData();

        pool.push([message{move(data_and_metadata.first)},metadata{data_and_metadata.second}, node_address_map, output_cache] (int thread_id) {
            gpu_raw_buffer_container serialized SerializerFunction(std::move(message));
            //TODO: this needs to change we need to make the container a struct
            std::vector<std::size_t> buffer_sizes = std::get<0>(serialized);
            std::vector<const char *> buffers = std::get<1>(serialized);
            std::vector<ColumnTransport> column_transports = std::get<2>(serialized);
			std::vector<std::unique_ptr<rmm::device_buffer>> column_scopes std::get<3>(serialized);

            try{
                //Initializes the sender with information needed for communicating the function that begins transmission
                //This constructor will make a ucx call iwth all the metadata and wait for it to complete
                Sender sender(node_address_map,metadata, buffer_sizes, column_transports); 
                
                for(size_t buffer_index = 0; buffer_index < buffers.size(); buffer_index++){
                    sender.send(
                        buffers[buffer_index],
                        buffer_sizes[buffer_index],
                    );
                    column_scopes[buffer_index] = nullptr; //allow the device_vector to go out of scope
                }

            }catch (ral::exception::communication_initialization e){

            }catch (ral::exception::communication_transmission e){
                
            }catch (std::exception e){

            }

        });

    }
}



class buffer_sender{
public:
    Sender(std::map<std::string, NodeAddress *> node_address_map,metadata,buffer_sizes,column_transports){
        //iterate for workers this is destined for
        for( auto worker_id : StringUtil::split(metadata[WORKER_IDS_METADATA_LABEL],",")){
            if(node_address_map.find(worker_id) == node_address_map.end()){
                throw std::exception(); //TODO: make a real exception here
            }
            destinations.push_back(node_address_mapp[worker_id])
        }
    }
protected:
    virtual void send_begin_transmission() = delete;
    virtual void send(const char * buffer, size_t buffer_size) = delete;
    std::pair<std::unique_ptr<buffer_frame>, frame_size > make_begin_transmission(){
        //builds the cpu host buffer that we are going to send
    }
private:
    std::vector<NodeAddress *> destinations; /**< The nodes that will be receiving these buffers */
}

template <typename DeserializerFunction>
class message_receiver{
    /**
    * Constructor for the message receiver.
    * This is a place for a message to receive chunks. It calls the deserializer after the complete 
    * message has been assembled
    * @param buffer_num_bytes tells the size in bytes of each buffer we will receive
    * @param column_transports This is metadata about how a column will be reconstructed used by the deserialzer
    * @param buffer_id_to_positions each buffer will have an id which lets us know what position it maps to in the 
    *                               list of buffers and the deserializer it will use. 
    * @param output_cache The destination for the message being received. It is either a specific cache inbetween
    *                     two kernels or it is intended for the general input cache using a mesage_id
    * @param metadata This is information about how the message was routed and payloads that are used in
    *                 execution, planning, or physical optimizations. E.G. num rows in table, num partitions to be processed
    * @param include_metadata Indicates if we should add the metadata along with the payload to the cache or if we should not.                    
    */
    message_receiver(std::vector<std::size_t> buffer_num_bytes, std::vector<ColumnTransport> column_transports,
        std::map<std::string,size_t> buffer_id_to_position, std::shared_ptr<ral::cache::CacheMachine> output_cache,
        ral::cache::MetadataDictionary metadata, bool include_metadata){

        }
protected:
    virtual void add_buffer(std::unique_ptr<rmm::device_buffer> buffer, std::string id){
        raw_buffers[buffer_id_to_position[id]] = std::move(buffer);
    }
    std::unique_ptr<ral::frame::BlazingTable> finish(){

        std::unique_ptr<ral::frame::BlazingTable> table = DeserializerFunction(column_transports,std::move(raw_buffers));
        if(include_metadata){
            output_cache->addCacheData(
                std::make_unique<ral::cache::GPUCacheDataMetaData>(
                    std::move(table), metadata),metadata[MESSAGE_ID],true);
        }else{
            output_cache->addToCache( std::move(table),"",true);
        }

    }
    std::vector<std::unique_ptr<rmm::device_buffer> > raw_buffers;

}

