# User Guide
# 1.python memory_display.py
# 2.Please enter the path to your .py file: ./cv181x/memmap_ddr_64mb.py
import sys
import math
import importlib.util
import os


def import_class_from_path(file_path, class_name):
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"The file '{file_path}' does not exist.")
    if not os.access(file_path, os.R_OK):
        raise PermissionError(f"The file '{file_path}' is not accessible.")
    spec = importlib.util.spec_from_file_location("module_name", file_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules["module_name"] = module
    spec.loader.exec_module(module)
    try:
        return getattr(module, class_name)
    except AttributeError:
        raise ImportError(f"The class '{class_name}' is not found in '{file_path}'.")


def calculate_endings(memory_map_class):
    attrs = dir(memory_map_class)
    for attr in attrs:
        if attr.endswith("_ADDR"):
            base_name = attr[:-5]  # Remove '_ADDR' from the end
            size_attr = f"{base_name}_SIZE"
            if size_attr in attrs:
                addr = getattr(memory_map_class, attr)
                size = getattr(memory_map_class, size_attr)
                end_attr = f"{base_name}_END"
                setattr(memory_map_class, end_attr, addr + size)


def display_all(memory_map_class):
    calculate_endings(memory_map_class)  # Calculate the _END attributes before displaying
    data = []  # Initialize an empty list to collect data
    attrs = dir(memory_map_class)
    for attr in attrs:
        if attr.endswith("_ADDR"):
            base_name = attr[:-5]  # Remove '_ADDR' from the end
            size_attr = f"{base_name}_SIZE"
            end_attr = f"{base_name}_END"
            if size_attr in attrs:
                start_address = math.floor(getattr(memory_map_class, attr))
                size = math.floor(getattr(memory_map_class, size_attr))
                end_address = math.floor(getattr(memory_map_class, end_attr))
            else:
                start_address = math.floor(getattr(memory_map_class, attr))
                size = 0
                end_address = start_address
            data.append({
                "Name": base_name,
                "Start Address": hex(start_address) if isinstance(start_address, int) else start_address,
                "End Address": hex(end_address) if isinstance(end_address, int) else end_address,
                "Size": hex(size) if isinstance(size, int) else size
            })
    return data


def sort_and_save_vertical_table(data, stage):
    def address_to_int(addr):
        if addr == "-":
            return float('-inf')
        return int(addr, 16)

    data.sort(key=lambda x: address_to_int(x["Start Address"]))

    columns = list(data[0].keys())

    output = ""
    for column in columns:
        output += f"| {column: <27} |"
        for item in data:
            output += f" {item[column]: <15} |"
        output += "\n" + "-" * (30 + 18 * len(data)) + "\n"
    output_filename = "memmap.txt"
    if stage == 1:
        with open(output_filename, 'w') as file:
            file.write('FBSL stage:\n')
            file.write(output)
            file.write('\n')
    elif stage == 2:
        with open(output_filename, 'a') as file:
            file.write('uboot stage:\n')
            file.write(output)
            file.write('\n')
    elif stage == 3:
        with open(output_filename, 'a') as file:
            file.write('kernel stage:\n')
            file.write(output)
            file.write('\n')
            file.write('PS: When the kernel is booted, it will overwrite the FSBL and uboot memory space')


def filter_first_stage(data):
    prefixes = ("FREERTOS", "UIMAG", "MONITOR", "OPENSBI", "FSBL", "ALIOS")
    first_stage = [item for item in data if item["Name"].startswith(prefixes)]
    return first_stage


def filter_second_stage(data):
    prefixes = ("BOOTLOGO", "CVI_UPDATE", "UIMAG", "MONITOR", "OPENSBI", "FREERTOS", "ALIOS")
    second_stage = [item for item in data if item["Name"].startswith(prefixes)]
    return second_stage


def filter_third_stage(data):
    prefixes = ("KERNEL", "MONITOR", "OPENSBI", "FREERTOS", "H26X", "ION",
                "ISP", "FRAMEBUFFER", "ALIOS", "SHARE", "PQBIN")
    third_stage = [item for item in data if item["Name"].startswith(prefixes)]
    return third_stage


def main():
    file_path = input("Please enter the path to your .py file: ")
    # class_name = input("Please enter the name of the class to import: ")
    # file_path = './cv180x/memmap_ddr_64mb.py'
    class_name = 'MemoryMap'
    try:
        MyClass = import_class_from_path(file_path, class_name)
        my_instance = MyClass()
        print(f"Successfully created an instance of '{class_name}' from '{file_path}'.")
        data = display_all(my_instance)
        first_stage = filter_first_stage(data)
        sort_and_save_vertical_table(first_stage, 1)

        second_stage = filter_second_stage(data)
        sort_and_save_vertical_table(second_stage, 2)

        third_stage = filter_third_stage(data)
        sort_and_save_vertical_table(third_stage, 3)
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    main()
