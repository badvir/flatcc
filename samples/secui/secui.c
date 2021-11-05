//
// Created by root on 11/2/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "secui_builder.h"

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(secui_tams, x) // Specified in the schema.

#define test_assert(x) do { if (!(x)) { assert(0); return -1; }} while(0)


int serialize_traffic_session(flatcc_builder_t *builder)
{
    int rval;

    // Initialize the builder object.
    flatcc_builder_init(builder);

    // start builder
    ns(traffic_session_start_as_root(builder));

    // mach_id
    ns(traffic_session_mach_id_add(builder, 1234));

    // src_ip
    struct in6_addr src_ip;
    if ( (rval = inet_pton(AF_INET6, "::192.168.1.1", &src_ip)) == 0) {
        printf("Invalid addresss\n");
        exit(EXIT_FAILURE);
    } else if (rval == -1) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    ns(ipaddress_t) *x_ip;
    x_ip = ns(traffic_session_src_ip_start(builder));
    memcpy(x_ip->ip, &src_ip, sizeof(struct in6_addr));
    ns(traffic_session_src_ip_end(builder));

    // src_port
    ns(traffic_session_src_port_add(builder, 5412));

    // dst_ip
    struct in6_addr dst_ip;
    if ( (rval = inet_pton(AF_INET6, "::10.10.1.1", &dst_ip)) == 0) {
        printf("Invalid addresss\n");
        exit(EXIT_FAILURE);
    } else if (rval == -1) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    x_ip = ns(traffic_session_dst_ip_start(builder));
    memcpy(x_ip->ip, &dst_ip, sizeof(struct in6_addr));
    ns(traffic_session_dst_ip_end(builder));

    // dst_port
    ns(traffic_session_dst_port_add(builder, 80));

    // detail_msg
    ns (traffic_session_detail_msg_create_str(builder, "test_messages"));

    // end builder
    ns(traffic_session_end_as_root(builder));
}


int deserialize_traffic_session(void *buffer)
{
    char ip_str[INET6_ADDRSTRLEN] = {0,};

    ns(traffic_session_table_t) traffic_session = ns(traffic_session_as_root(buffer));
    test_assert(traffic_session != 0);

    // mach_id
    int32_t mach_id = ns(traffic_session_mach_id(traffic_session));
    printf("mach_id: %d\n", mach_id);

    // src_ip
    ns(ipaddress_struct_t) x_src_ip = ns(traffic_session_src_ip(traffic_session));
    struct in6_addr src_ip;
    memcpy(&src_ip, x_src_ip->ip, sizeof(struct in6_addr));
    inet_ntop(AF_INET6, &src_ip, ip_str, INET6_ADDRSTRLEN);
    printf("src_ip: %s\n", ip_str);

    // src_port
    uint16_t src_port = ns(traffic_session_src_port(traffic_session));
    printf("src_port: %u\n", src_port);

    // dst_ip
    ns(ipaddress_struct_t) x_dst_ip = ns(traffic_session_dst_ip(traffic_session));
    struct in6_addr dst_ip;
    memcpy(&dst_ip, x_dst_ip->ip, sizeof(struct in6_addr));
    inet_ntop(AF_INET6, &dst_ip, ip_str, INET6_ADDRSTRLEN);
    printf("dst_ip: %s\n", ip_str);

    // dst_port
    uint16_t dst_port = ns(traffic_session_dst_port(traffic_session));
    printf("dst_port: %u\n", dst_port);

    // detail_msg
    flatbuffers_string_t detail_msg = ns(traffic_session_detail_msg(traffic_session));
    size_t detail_msg_len = flatbuffers_string_len(detail_msg);
    printf("detail_msg: %s, size: %d\n", detail_msg, detail_msg_len);

    // test
    //uint16_t test = ns(traffic_session_test(traffic_session));
    //printf("test: %u\n", test);

}

int save_file(const char* file_path, char* buffer, size_t size)
{
    FILE *fp;
    fp = fopen(file_path,"wb");
    if (fp == NULL) {
        printf("err: %s\n", strerror(errno));
        return -1;
    }


    size_t rwsize = fwrite(buffer, 1, size, fp);
    if (rwsize <= 0) {
        fclose(fp);
        printf("err: %s\n", strerror(errno));
        return -1;
    }

    fclose(fp);
    return rwsize;
}


size_t load_file(const char *file_path, char *buffer, size_t size)
{
    FILE *fp;
    fp = fopen(file_path,"rb");
    if (fp == NULL)
        return -1;

    size_t rwsize = fread(buffer, 1, size, fp);

    if(rwsize <= 0){
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return rwsize;
}

int test_all()
{
    void  *buf;
    size_t size;

    flatcc_builder_t builder;

    // serialize
    serialize_traffic_session(&builder);

    // serialize to buffer
    buf = flatcc_builder_finalize_aligned_buffer(&builder, &size);

    printf("serialized size: %zu\n", size);

    // deserialize
    deserialize_traffic_session(buf);

    // destory
    flatcc_builder_aligned_free(buf);
    flatcc_builder_clear(&builder);
}

int test_save_file(const char* file_path)
{
    void  *buf;
    size_t size;

    flatcc_builder_t builder;

    // serialize
    serialize_traffic_session(&builder);

    // serialize to buffer
    buf = flatcc_builder_finalize_aligned_buffer(&builder, &size);

    int ret = save_file(file_path, buf, size);
    printf("ret: %d\n", ret);

    // destory
    flatcc_builder_aligned_free(buf);
    flatcc_builder_clear(&builder);

    return ret;
}

int test_read_file(const char* file_path)
{
    char buf[1024] = {0,};
    size_t size;

    //flatcc_builder_t builder;

    int ret = load_file(file_path, buf, sizeof(buf));
    if (ret <= 0) {
        printf("load_file() failed\n");
        return -1;
    }

    // deserialize
    deserialize_traffic_session((void *)buf);

    // destory
    //flatcc_builder_clear(&builder);


    return ret;
}

int main()
{
    //test_save_file("traffic_session.data");
    //test_read_file("traffic_session.data");
    test_all();

}


