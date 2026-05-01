pub struct DtboInfo {
    pub magic: u32,
    pub total_size: u32,
    pub header_size: u32,
    pub dt_entry_size: u32,
    pub dt_entry_count: u32,
}

pub fn parse_dtbo_header(data: &[u8]) -> Result<DtboInfo, String> {
    if data.len() < 28 {
        return Err("File too small for DTBO header".into());
    }
    let get_u32 =
        |off: usize| u32::from_be_bytes([data[off], data[off + 1], data[off + 2], data[off + 3]]);
    let magic = get_u32(0);
    if magic != 0xd7b7ab1e {
        return Err(format!("Invalid DTBO magic: 0x{:08x}", magic));
    }
    Ok(DtboInfo {
        magic,
        total_size: get_u32(4),
        header_size: get_u32(8),
        dt_entry_size: get_u32(12),
        dt_entry_count: get_u32(20),
    })
}
