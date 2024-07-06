<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="450" height="100">
  </a>

  <p align="center">
    Fileship is an open-source and self-hosted image uploading service with cool features.
    <br />
    <br />
    <a href="https://fileship.hanzy.dev">Docs</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues">Report Bug</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues">Request Feature</a>
  </p>
</div>

### Built With

![Nuxt.js](https://img.shields.io/static/v1?style=for-the-badge&message=Nuxt.js&color=222222&logo=Nuxt.js&logoColor=00DC82&label=)
![Vue.js](https://img.shields.io/static/v1?style=for-the-badge&message=Vue.js&color=222222&logo=Vue.js&logoColor=4FC08D&label=)
![UnoCSS](https://img.shields.io/static/v1?style=for-the-badge&message=UnoCSS&color=333333&logo=UnoCSS&logoColor=FFFFFF&label=)
![Prisma](https://img.shields.io/static/v1?style=for-the-badge&message=Prisma&color=2D3748&logo=Prisma&logoColor=FFFFFF&label=)
![TypeScript](https://img.shields.io/static/v1?style=for-the-badge&message=TypeScript&color=3178C6&logo=TypeScript&logoColor=FFFFFF&label=)
![Socket.io](https://img.shields.io/static/v1?style=for-the-badge&message=Socket.io&color=010101&logo=Socket.io&logoColor=FFFFFF&label=)
![ESLint](https://img.shields.io/static/v1?style=for-the-badge&message=ESLint&color=4B32C3&logo=ESLint&logoColor=FFFFFF&label=)
![Prettier](https://img.shields.io/static/v1?style=for-the-badge&message=Prettier&color=222222&logo=Prettier&logoColor=F7B93E&label=)
![Vite](https://img.shields.io/static/v1?style=for-the-badge&message=Vite&color=646CFF&logo=Vite&logoColor=FFFFFF&label=)

<!-- GETTING STARTED -->

## Getting Started

To get a local copy up and running follow these simple steps.

### Install & run using Docker

This section requires [Docker](https://www.docker.com/) and [Docker Compose](https://docs.docker.com/compose/) to be installed on your machine.

1. Clone the repo
    ```sh
    git clone https://github.com/hanzydev/Fileship.git
    ```
2. Go into the repo directory
    ```sh
    cd Fileship
    ```
3. Run the following command to start the server
    ```sh
    docker-compose up -d
    ```
4. Open your browser and go to `http://localhost:3000`

### Building & running from source

This section requires [Node.js](https://nodejs.org/) and [pnpm](https://pnpm.io/) to be installed on your machine.

1. Clone the repo
    ```sh
    git clone https://github.com/hanzydev/Fileship.git
    ```
2. Go into the repo directory
    ```sh
    cd Fileship
    ```
3. Install NPM packages
    ```sh
    pnpm i
    ```
4. Create an .env file using .env.example. The only one required is the `DATABASE_URL` which is the connection string to your database.

    ```sh
    cp .env.example .env
    ```

5. Build the project
    ```sh
    pnpm build
    ```
6. Start the server
    ```sh
    pnpm preview
    ```
7. Open your browser and go to `http://localhost:3000`

<!-- LICENSE -->

## License

Distributed under the MIT License. See `LICENSE` for more information.

<!-- ALL CONTRIBUTORS HERE -->

## Documentation

The documentation for Fileship can be found [here](https://fileship.hanzy.dev).
