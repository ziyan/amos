// arduino combines all pde files, so all include should be in base.pde

#define COMM_STATE_STX    0
#define COMM_STATE_HEAD   1
#define COMM_STATE_DATA   2
#define COMM_STATE_CRC    3

uint8_t comm_state = COMM_STATE_STX;
uint8_t comm_state_data_count = 0;

// comm packet buffer
packet_t comm_packet = {0};

void comm_setup()
{
    Serial.begin(115200);
}

// dispatch and handle packet
int8_t comm_handle_packet(packet_t *pkt)
{
    switch (pkt->cmd)
    {
        case 'R':
            return comm_reset_handler(pkt);
        case 'S':
            return comm_set_speed_handler(pkt);
        case 'P':
            return comm_set_pid_handler(pkt);
        case 'A':
            return comm_set_acceleration_handler(pkt);
        case 'o':
            return comm_get_odometry_handler(pkt);
        case 'p':
            return comm_get_power_handler(pkt);
        default:
            return -PACKET_ERR_NOT_IMPLEMENTED; // command not implemented
    }
}

// this function does a nonblocking processing for new packets
void comm_run()
{
    uint8_t i = 0, stx = 0;
    uint16_t crc = 0;
    int rc = 0, b = 0;

    switch (comm_state)
    {
        case COMM_STATE_STX:
        {
            if (Serial.available() < 1) break;
            stx = Serial.read();
            if (stx != PACKET_STX) break;
            comm_state = COMM_STATE_HEAD;
            break;
        }
        case COMM_STATE_HEAD:
        {
            if (Serial.available() < 2) break;
            comm_packet.cmd = Serial.read();
            comm_packet.len = Serial.read();
            comm_state_data_count = 0;
            if (comm_packet.len > PACKET_DATA_MAX_LEN)
                comm_state = COMM_STATE_STX;
            else
                comm_state = COMM_STATE_DATA;
            break;
        }
        case COMM_STATE_DATA:
        {
            // now get the data portion
            while (comm_packet.len > comm_state_data_count)
            {
                b = Serial.read();
                if (b < 0) break;
                comm_packet.data[comm_state_data_count] = b;
                comm_state_data_count++;
            }

            if (comm_packet.len <= comm_state_data_count)
                comm_state = COMM_STATE_CRC;
            break;
        }
        case COMM_STATE_CRC:
        {
            if (Serial.available() < 2) break;
            crc = Serial.read() << 8;
            crc |= Serial.read();
            
            // see if packet is valid, ignore invalid packet
            if (crc_compute((uint8_t*)(&comm_packet), comm_packet.len + 2) == crc)
            {
                // handle the packet
                rc = comm_handle_packet(&comm_packet);
                // see if there is an error
                if (rc || comm_packet.len > PACKET_DATA_MAX_LEN)
                {
                    // prepare nack to be sent
                    comm_packet.cmd = PACKET_CMD_NCK;
                    comm_packet.len = 1;
                    comm_packet.data[0] = rc;
                }
                
                // send response
                crc = crc_compute((uint8_t*)(&comm_packet), comm_packet.len + 2);
                Serial.write(PACKET_STX);
                Serial.write((uint8_t*)(&comm_packet), comm_packet.len + 2);
                Serial.write((crc >> 8) & 0xFF);
                Serial.write(crc & 0xFF);
            }
            comm_state = COMM_STATE_STX;
            break;
        }
        default:
            comm_state = COMM_STATE_STX;
            break;
    }
}


// handles reset command
int8_t comm_reset_handler(packet *pkt)
{
    if (pkt->len != 0)
        return -PACKET_ERR_INVALID_FORMAT;
    Motor::left.reset();
    Motor::right.reset();
    return 0;
}

// handles set motor speed command
int8_t comm_set_speed_handler(packet *pkt)
{
    typedef struct {
        float left, right;
    } data_t;
    
    if (pkt->len != sizeof(data_t))
        return -PACKET_ERR_INVALID_FORMAT;
    data_t *data = (data_t*)pkt->data;

    Motor::left.setSpeed(data->left);
    Motor::right.setSpeed(data->right);
    
    pkt->len = 0;
    return 0;
}


// handles set PID parameters
int8_t comm_set_pid_handler(packet *pkt)
{
    typedef struct {
        float p_left, i_left, d_left;
      	float p_right, i_right, d_right;
    } data_t;
    
    if (pkt->len != sizeof(data_t))
        return -PACKET_ERR_INVALID_FORMAT;
    data_t *data = (data_t*)pkt->data;
    
    Motor::left.setPID(data->p_left, data->i_left, data->d_left);
    Motor::right.setPID(data->p_right, data->i_right, data->d_right);
    
    pkt->len = 0;
    return 0;
}


// handles set acceleration limit command
int8_t comm_set_acceleration_handler(packet *pkt)
{
    typedef struct {
        float a_left, a_right;
    } data_t;
    
    if (pkt->len != sizeof(data_t))
        return -PACKET_ERR_INVALID_FORMAT;
    data_t *data = (data_t*)pkt->data;
    
    Motor::left.setAcceleration(data->a_left);
    Motor::right.setAcceleration(data->a_right);
    
    pkt->len = 0;
    return 0;
}


// handles read odometry command
int8_t comm_get_odometry_handler(packet *pkt)
{
    typedef struct {
        float left, left_t, right, right_t;
    } data_t;
    
    if (pkt->len != 0)
        return -PACKET_ERR_INVALID_FORMAT;

    pkt->len = sizeof(data_t);
    data_t *data = (data_t*)pkt->data;
    
    Motor::left.getOdometry(&data->left, &data->left_t);
    Motor::right.getOdometry(&data->right, &data->right_t);    
    return 0;
}


// handles read power information command
int8_t comm_get_power_handler(packet *pkt)
{
    typedef struct {
        float voltage_left, current_left, voltage_right, current_right;
    } data_t;
    
    if (pkt->len != 0)
        return -PACKET_ERR_INVALID_FORMAT;

    pkt->len = sizeof(data_t);
    data_t *data = (data_t*)pkt->data;
    
    data->voltage_left = Motor::left.getVoltage();
    data->current_left = Motor::left.getCurrent();
    data->voltage_right = Motor::right.getVoltage();
    data->current_right = Motor::right.getCurrent();
    return 0;
}

