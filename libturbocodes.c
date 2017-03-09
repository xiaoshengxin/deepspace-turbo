//
// Created by gianluca on 22/02/17.
//

#include "libturbocodes.h"
#include <stdlib.h>
#include "utilities.h"
#include <stdio.h>


t_turbocode turbo_initialize(t_convcode *codes, int components, int **interleaver, int packet_length)
{
    t_turbocode code;/*{{{*/
    code.components = components;
    code.inner_codes = codes;
    code.packet_length = packet_length;
    code.interleaving_vectors = interleaver;

    // compute encoded length
    int turbo_length = 0;

    for (int i = 0; i < code.components; ++i) {
        // get corresponding (possibly) interleaved input message
        t_convcode cc = code.inner_codes[i];
        turbo_length += cc.components * (code.packet_length + cc.memory);
    }

    code.encoded_length = turbo_length;

    return code;/*}}}*/
}

static int **turbo_interleave(int *packet, t_turbocode code)
{
    int **interleaved_messages = malloc(code.components * sizeof(int*));/*{{{*/
    for (int i = 0; i < code.components; ++i) {
        interleaved_messages[i] = malloc(code.packet_length * sizeof(int));/*{{{*/
        for (int j = 0; j < code.packet_length; ++j) {
            interleaved_messages[i][j] = packet[code.interleaving_vectors[i][j]];
        }/*}}}*/
    }

    return interleaved_messages;/*}}}*/
}

int *turbo_encode(int *packet, t_turbocode code)
{
    int **interleaved_packets = turbo_interleave(packet, code);/*{{{*/

    // reference to encoded messages
    int **conv_encoded = malloc(code.components * sizeof(int*));
    int turbo_length = code.encoded_length;

    for (int i = 0; i < code.components; ++i) {
        // get corresponding (possibly) interleaved input message
        t_convcode cc = code.inner_codes[i];
        int *interleaved_pkt = interleaved_packets[i];

        conv_encoded[i] = convcode_encode(interleaved_pkt, code.packet_length, cc);
    }

    int *turbo_encoded = malloc(turbo_length * sizeof *turbo_encoded);

    // parallel to serial
    int k = 0, c = 0, cw = 0;/*{{{*/
    while (k < turbo_length) {
        t_convcode cc = code.inner_codes[c];
        
        // number of components of cc
        int comps = cc.components;

        // copy bits from cc output to turbo_encoded
        for (int i = 0; i < comps; i++) {
            int bit = conv_encoded[c][cw*comps + i];
           turbo_encoded[k++] = bit;
        }
        
        c = (c + 1) % code.components;
        // when c = 0 the first codeword is complete
        cw = !c  ? cw+1 : cw;
    }/*}}}*/

    for (int i = 0; i < code.components; i++)
       free(interleaved_packets[i]) ;
    free(interleaved_packets);

    return turbo_encoded;/*}}}*/
}

int *turbo_decode(double *received, t_turbocode code)
{
    // serial to parallel/*{{{*/
    int *lengths = malloc(code.components * sizeof  *lengths);/*{{{*/
    double **streams = malloc(code.components * sizeof(double*));
    for (int i = 0; i < code.components; i++) {
        t_convcode cc = code.inner_codes[i];
        lengths[i] = cc.components * (code.packet_length + cc.memory);
        streams[i] = malloc(lengths[i] * sizeof(double));
    }
    
    int k = 0, c = 0, cw = 0;
    while (k < code.encoded_length) {
        t_convcode cc = code.inner_codes[c];

        for (int i = 0; i < cc.components; i++)
           streams[c][cw*cc.components + i] = received[k++];

        c = (c + 1) % code.components;
        cw = !c ? cw + 1 : cw;
    }/*}}}*/

    int *turbo_decoded = malloc(code.packet_length * sizeof *turbo_decoded);

    for (int i = 0; i < code.components; i++)
        free(streams[i]);
    free(streams);

    return turbo_decoded; /*}}}*/
}
