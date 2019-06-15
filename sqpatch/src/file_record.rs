use crate::{Parse, Position, Stream};

use std::cmp::min;

use byteorder::{ReadBytesExt, BE, LE};

pub struct FileRecord {
    pub header: [u8; 4],
    pub unk0: u32,
    pub file_offset: u32,
    pub unk1: u32,
    pub file_length: u32,
    pub unk2: u32,
    pub file_name: String,
    pub chunks: Vec<FileChunk>,
    pub hash: [u8; 4],
}

impl Parse for FileRecord {
    fn parse(stream: &mut impl Stream) -> std::io::Result<FileRecord> {
        let mut header = [0; 4];
        stream.read_exact(&mut header)?;
        
        let unk0 = stream.read_u32::<BE>()?;
        let file_offset = stream.read_u32::<BE>()?;
        let unk1 = stream.read_u32::<BE>()?;

        let file_length = stream.read_u32::<BE>()?;
        let file_name_len = stream.read_u32::<BE>()?;
        let unk2 = stream.read_u32::<BE>()?;

        let mut file_name = vec![0; file_name_len as usize];
        stream.read_exact(&mut file_name)?;
        file_name.pop();
        let file_name = String::from_utf8_lossy(&file_name).into_owned();

        let mut chunks = vec![];
        loop {
            if stream.peek_u32_le()? != 16 {
                break;
            }

            chunks.push(FileChunk::parse(stream)?);
        }

        let mut hash = [0; 4];
        stream.read_exact(&mut hash)?;

        Ok(FileRecord {
            header,
            unk0,
            file_offset,
            unk1,
            file_length,
            unk2,
            file_name,
            chunks,
            hash,
        })
    }
}

pub struct FileChunk {
    pub unk0: u32,
    pub unk1: u32,
    pub data_len: u32,
    pub expanded_len: u32,
    pub data: Position,
}

impl Parse for FileChunk {
    fn parse(stream: &mut impl Stream) -> std::io::Result<FileChunk> {
        let begin = stream.pos()?;

        let unk0 = stream.read_u32::<LE>()?;
        let unk1 = stream.read_u32::<LE>()?;

        let data_len = stream.read_u32::<LE>()?;
        let expanded_len = stream.read_u32::<LE>()?;

        let data = stream.pos()?;

        stream.seek_by(min(data_len, expanded_len) as i64)?;
        stream.align_up_from(begin, 0x80)?;

        Ok(FileChunk {
            unk0,
            unk1,
            data_len,
            expanded_len,
            data,
        })
    }
}
