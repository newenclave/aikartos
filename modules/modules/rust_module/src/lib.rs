#![no_std]
#![no_main]

use core::ffi::c_void;
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[repr(C)]
pub struct ModuleInfo {
    module_base: usize,
    module_size: usize,
}

#[repr(C)]
pub struct DeviceApi {
    uart_read: Option<unsafe extern "C" fn(*mut u8, usize) -> usize>,
    uart_write: Option<unsafe extern "C" fn(*const u8, usize)>,
}

#[repr(C)]
pub struct ThisTaskApi {
    sleep: Option<unsafe extern "C" fn(u32)>,
    yield_: Option<unsafe extern "C" fn()>,
}

#[repr(C)]
pub struct KernelApi {
    add_task: Option<unsafe extern "C" fn(fn(*mut c_void), *mut c_void)>,
}

#[repr(C)]
pub struct AikaApi {
    module: ModuleInfo,
    memory: [usize; 3],
    kernel: KernelApi,
    device: DeviceApi,
    this_task: ThisTaskApi,
    fpu: [usize; 2],
}

#[no_mangle]
pub extern "C" fn module_entry(api: *mut AikaApi) -> i32 {
    unsafe {
        if let Some(write_fn) = (*api).device.uart_write {
            let msg = b"\r\nHello from Rust module!\r\n\0";
            write_fn(msg.as_ptr(), msg.len());
        }

        if let Some(sleep_fn) = (*api).this_task.sleep {
            loop {
                let msg = b"Tick from Rust module\r\n\0";
                if let Some(write_fn) = (*api).device.uart_write {
                    write_fn(msg.as_ptr(), msg.len());
                }
                sleep_fn(1000);
            }
        }
    }
    0
}
