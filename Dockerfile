FROM devkitpro/devkita64:latest

WORKDIR /workspace
COPY . .

RUN make -j"$(nproc)" dist

CMD ["make", "dist"]
