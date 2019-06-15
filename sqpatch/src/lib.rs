pub use crate::{
    aply::Aply,
    container::Container,
    file_record::FileRecord,
    sqpk::{PackType, Sqpk},
    patch_file::PatchFile,
};

mod helpers;
pub mod aply;
pub mod container;
pub mod file_record;
pub mod sqpk;
pub mod patch_file;

#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Position(pub u64);

pub trait Stream: std::io::Read + std::io::Seek {
    fn pos(&mut self) -> std::io::Result<Position> {
        Ok(Position(self.seek(std::io::SeekFrom::Current(0))?))
    }

    fn seek_to(&mut self, pos: Position) -> std::io::Result<u64> {
        self.seek(std::io::SeekFrom::Start(pos.0))
    }

    fn seek_by(&mut self, by: i64) -> std::io::Result<u64> {
        self.seek(std::io::SeekFrom::Current(by))
    }

    fn align_up_from(&mut self, pos: Position, alignment: u64) -> std::io::Result<u64> {
        let curr = self.pos()?;
        let diff = curr.0.wrapping_sub(pos.0);
        let new_pos = Position(curr.0 + alignment - (diff % alignment));
        self.seek_to(new_pos)
    }

    fn peek_u32_le(&mut self) -> std::io::Result<u32> {
        use byteorder::{ReadBytesExt,LE};
        let cur = self.pos()?;
        let val = self.read_u32::<LE>()?;
        self.seek_to(cur)?;
        Ok(val)
    }
}

impl<R> Stream for R where R: std::io::Read + std::io::Seek {}

pub trait Parse: Sized {
    fn parse(stream: &mut impl Stream) -> std::io::Result<Self>;
}
