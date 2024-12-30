import matplotlib.pyplot as plt

def parse_file(filename):
    data = [0]
    times = [0]
    delay_sms = [0]
    delay_mss = [0]
    time = 0
    min = -100
    count = 0
    i =0
    with open(filename, 'r') as file:
        for line in file:
            items = line.strip().split(',')
            i = i + 1
            if len(items) >= 6:
                try:
                    # 假设第一列是时间，第六列是需要抓取的数据
                    time = 0.05 + time
                    value = float(items[4])
                    if(abs(min) > abs(value)):
                        if (value != 0.0) :
                            min = value
                    if(i > 0): # drop some start points
                        if(abs(value) < 0.0005):
                            count = count + 1

                    delay_ms = float(items[-2])
                    delay_sm = float(items[-1])
                    times.append(time)
                    data.append(value)
                    delay_mss.append(delay_ms)
                    delay_sms.append(delay_sm)
                except ValueError:
                    # 如果转换失败，跳过该行
                    continue
    return times, data, delay_mss, delay_sms, min, count

def plot_offset_data(times, data):
    plt.figure(figsize=(10, 6))
    plt.plot(times, data, marker='o')
    plt.xlabel('Time')
    plt.ylabel('Value (6th column)')
    plt.title('Plot of 6th Column Data Over Time')
    plt.grid(True)
    plt.show()

def plot_delay_data(times, delay_ms, delay_sm):
    plt.figure(figsize=(10, 6))
    plt.plot(times, delay_ms, marker='o', label='delay_MS', linestyle='--', color='r')
    plt.plot(times, delay_sm, marker='s', label='delay_SM', linestyle=':', color='green')
    plt.xlabel('Time')
    plt.ylabel('Value (6th column)')
    plt.title('Plot of 6th Column Data Over Time')
    plt.grid(True)
    plt.show()

def plot_all(times, offset, delay_ms, delay_sm):
    fig, axs = plt.subplots(3, 1, figsize=(10, 12))

    # plot ofset slave - master
    axs[0].plot(times, offset, label='offset', color='blue')
    axs[0].set_xlabel('x')
    axs[0].set_ylabel('y')
    axs[0].set_title('slave - master')
    axs[0].grid(True)
    axs[0].legend()

    # plot delay master to slave
    axs[1].plot(times, delay_ms, label='delay MS', color='green')
    axs[1].set_xlabel('x')
    axs[1].set_ylabel('y')
    axs[1].set_title('delay MS')
    axs[1].grid(True)
    axs[1].legend()

    # plot delay slave to master
    axs[2].plot(times, delay_sm, label='delay sm', color='red')
    axs[2].set_xlabel('x')
    axs[2].set_ylabel('y')
    axs[2].set_title('delay SM')
    axs[2].grid(True)
    axs[2].legend()

    # 
    plt.tight_layout()

    #
    plt.show()

if __name__ == "__main__":
    filename = r'D:\python_prj\1\1_ptp.txt'  # replace your file name
    times, offset, delay_ms, delay_sm, min, count = parse_file(filename)
    print('offset min is {0}, less than 500us counter is {1}, all point is : {2}, percent is :{3}'.format(min, count, len(offset), count*1.0/len(offset)))
    #plot_offset_data(times, offset) # ofset
    #plot_delay_data(times,delay_ms, delay_sm) # delay slave to master
    #plot_delay_data(times, delay_sm) #delay slave to masers
    plot_all(times,offset, delay_ms, delay_sm)
